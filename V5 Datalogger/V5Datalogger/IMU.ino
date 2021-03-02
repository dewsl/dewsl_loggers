/*
IMU code v1.3
*/
char IMUdataToSend[200];

const float div_accel = 10000.0, div_default = 100.0;

int *raw_data = (int *)malloc(sizeof(int) * 9);
int *calib_data = (int *)malloc(sizeof(int) * 9);
int *ave_data = (int *)malloc(sizeof(int) * 9);

int *a_param = (int *)malloc(sizeof(int) * 12);
int *m_param = (int *)malloc(sizeof(int) * 12);
int *g_param = (int *)malloc(sizeof(int) * 3);

const int num_of_ave_data = 100, num_of_ave_calib = 1000;

imu_calib nxp;

void on_IMU()
{
  /**Current consumption of bare IMU when sampling
   * 3.3V ~ 2.8mA
  */
  Serial.println("Turning ON IMU sensor");
  // pinMode(IMU_POWER, OUTPUT);
  // delay(100);
  // turn_ON_due(0); //added for testing only
  digitalWrite(IMU_POWER, HIGH);
  delay_millis(100);
  init_IMU();
  delay_millis(100);
}

void off_IMU()
{
  Serial.println("Turning OFF IMU sensor");
  delay_millis(100);
  digitalWrite(IMU_POWER, LOW);
  // turn_OFF_due(0);  //added for testing only
  // pinMode(IMU_POWER, INPUT);
}

/**Read Sensor IMU Data
 * calibrated: 0 or 1
*/
char *read_IMU_data(int calib)
{
  int *data;
  char str[20];
  char tmp[10];
  readTimeStamp();
  
  for (int i = 0; i < 200; i++)
    IMUdataToSend[i] = 0x00;

  if (get_calib_param() == 1)
  {
    data = get_calib_data();
  }
  else
    data = get_raw_data();
  /* MADSTA*R*accelerometer(x,y,z),magnetometer(x,y,z), gyro(x,y,z), 200901142120*/
  if (get_logger_version() == 10)
  {
    //send to LoRa
    strncpy(IMUdataToSend, ">>", 2);
    strncat((IMUdataToSend), (get_logger_A_from_flashMem()), (20));
  }
  else
  {
    //send to GSM
    strncpy((IMUdataToSend), (get_logger_A_from_flashMem()), (20));
  }

  if (get_calib_param() == 1)
  {
    strncat(IMUdataToSend, "*F*", 3);
  }
  else
  {
    strncat(IMUdataToSend, "*R*", 3);
  }
  for (int i = 0; i < 9; i++)
  {
    if (i < 6)
      sprintf(str, "%.4f", ((float)data[i]) / div_accel);
    else
      sprintf(str, "%.2f", ((float)data[i]) / div_default);

    strncat(IMUdataToSend, str, String(str).length() + 1);
    strncat(IMUdataToSend, ",", 1);
  }
  snprintf(tmp, sizeof tmp, "%.2f", readTemp());
  strncat(IMUdataToSend, tmp, sizeof(tmp));

  strncat(IMUdataToSend, ",", 1);
  strncat(IMUdataToSend, Ctimestamp, sizeof(Ctimestamp));
  delay_millis(100);
  Serial.println(IMUdataToSend);
  return IMUdataToSend;
}

void init_IMU()
{
  /* Initialise the sensor */
  if (!gyro.begin())
  {
    /* There was a problem detecting the FXAS21002C ... check your connections*/
    Serial.println("Ooops, no FXAS21002C detected ... Check your wiring!");
    // while (1);
  }
  if (!accelmag.begin(ACCEL_RANGE_2G))
  {
    /* There was a problem detecting the FXOS8700 ... check your connections */
    Serial.println("Ooops, no FXOS8700 detected ... Check your wiring!");
    // while (1);
  }
}

void sensor_get_data()
{
  /* Get a new sensor event */
  sensors_event_t event, aevent, mevent;

  gyro.getEvent(&event);
  /* Get a new sensor event */
  accelmag.getEvent(&aevent, &mevent);
  int temp = 0;

  /* Display the accel results (acceleration is measured in m/s^2) */
  //  Serial.print("A ");
  //  Serial.print("X: ");
  Serial.print(aevent.acceleration.x, 4);
  Serial.print(",");
  //  Serial.print("Y: ");
  Serial.print(aevent.acceleration.y, 4);
  Serial.print(",");
  //  Serial.print("Z: ");
  Serial.print(aevent.acceleration.z, 4);
  Serial.print(",");
  //  Serial.println("m/s^2");

  /* Display the mag results (mag data is in uTesla) */
  //  Serial.print("M ");
  //  Serial.print("X: ");
  Serial.print(mevent.magnetic.x, 1);
  Serial.print(",");
  //  Serial.print("Y: ");
  Serial.print(mevent.magnetic.y, 1);
  Serial.print(",");
  //  Serial.print("Z: ");
  Serial.print(mevent.magnetic.z, 1);
  Serial.print(",");
  //  Serial.println("uT");

  /* Display the results (speed is measured in rad/s) */
  //  Serial.print("X: ");
  Serial.print(event.gyro.x);
  Serial.print(",");
  //  Serial.print("Y: ");
  Serial.print(event.gyro.y);
  Serial.print(",");
  //  Serial.print("Z: ");
  Serial.print(event.gyro.z);
  Serial.print(",");
  Serial.print(temp);

  Serial.println("");
}

int *get_raw_data()
{
  int *data = raw_data;
  /* Get a new sensor event */
  sensors_event_t event, aevent, mevent;

  gyro.getEvent(&event);
  /* Get a new sensor event */
  accelmag.getEvent(&aevent, &mevent);

  //accel data
  data[0] = (int)(aevent.acceleration.x * 10000.0);
  data[1] = (int)(aevent.acceleration.y * 10000.0);
  data[2] = (int)(aevent.acceleration.z * 10000.0);

  //magnetometer data
  data[3] = (int)(mevent.magnetic.x * 10000.0);
  data[4] = (int)(mevent.magnetic.y * 10000.0);
  data[5] = (int)(mevent.magnetic.z * 10000.0);

  //gyro data
  data[6] = (int)(event.gyro.x * 100.0);
  data[7] = (int)(event.gyro.y * 100.0);
  data[8] = (int)(event.gyro.z * 100.0);

  return data;
}

int *get_calib_data()
{
  //  int *calib_data = (int*)malloc(sizeof(int)*9);
  //  int *raw_data; // = (int*)malloc(sizeof(int)*9);
  //  int *ave_data = (int*)malloc(sizeof(int)*9);
  for (int j = 0; j < 9; j++)
    ave_data[j] = 0;

  int *a_param; // = (int*)malloc(sizeof(int)*12);
  int *m_param; // = (int*)malloc(sizeof(int)*12);
  int *g_param; // = (int*)malloc(sizeof(int)*3);
  /* Get a new sensor event */

  //average raw data
  for (int i = 0; i < num_of_ave_data; i++)
  {
    raw_data = get_raw_data();
    for (int j = 0; j < 9; j++)
      ave_data[j] = ave_data[j] + raw_data[j];
    delay_millis(1);
  }

  for (int j = 0; j < 9; j++)
    ave_data[j] = ave_data[j] / num_of_ave_data;
  //raw_data = ;

  a_param = get_calib_param_from_flashMem(1);
  calib_data[0] = (float)(a_param[0] * (ave_data[0] - a_param[9]) + a_param[1] * (ave_data[1] - a_param[10]) + a_param[2] * (ave_data[2] - a_param[11])) / 10000.0;
  calib_data[1] = (float)(a_param[3] * (ave_data[0] - a_param[9]) + a_param[4] * (ave_data[1] - a_param[10]) + a_param[5] * (ave_data[2] - a_param[11])) / 10000.0;
  calib_data[2] = (float)(a_param[6] * (ave_data[0] - a_param[9]) + a_param[7] * (ave_data[1] - a_param[10]) + a_param[8] * (ave_data[2] - a_param[11])) / 10000.0;

  m_param = get_calib_param_from_flashMem(2);
  calib_data[3] = (float)(m_param[0] * (ave_data[3] - m_param[9]) + m_param[1] * (ave_data[4] - m_param[10]) + m_param[2] * (ave_data[5] - m_param[11])) / 10000.0;
  calib_data[4] = (float)(m_param[3] * (ave_data[3] - m_param[9]) + m_param[4] * (ave_data[4] - m_param[10]) + m_param[5] * (ave_data[5] - m_param[11])) / 10000.0;
  calib_data[5] = (float)(m_param[6] * (ave_data[3] - m_param[9]) + m_param[7] * (ave_data[4] - m_param[10]) + m_param[8] * (ave_data[5] - m_param[11])) / 10000.0;

  g_param = get_calib_param_from_flashMem(3);
  calib_data[6] = ave_data[6] - g_param[0];
  calib_data[7] = ave_data[7] - g_param[1];
  calib_data[8] = ave_data[8] - g_param[2];

  return calib_data;
}

//if sensor = 1 = accel, 2= magnetometer, 3= gyroscope
int *get_calib_param_from_flashMem(int sensor)
{
  char *param;
  int i = 0, divisor, num_of_param;
  int *calib_param;
  String str;

  imu_calib calib_read = flash_imu_calib.read();

  switch (sensor)
  {
  case 1: //accel
    param = calib_read.accel_param;
    calib_param = a_param;
    num_of_param = 12;
    divisor = div_accel;
    break;

  case 2: //magnetometer
    param = calib_read.magneto_param;
    calib_param = m_param;
    num_of_param = 12;
    divisor = div_accel;
    break;

  case 3: //gyroscope
    param = calib_read.gyro_param;
    calib_param = g_param;
    num_of_param = 3;
    divisor = div_default;
    break;
  }

  while ((str = strtok_r(param, ";", &param)) != NULL)
  { // delimiter is the semicolon
    //Serial.println(str);
    calib_param[i] = (int)(str.toFloat() * divisor);
    i++;
  }

  //  for (int j=0; j<num_of_param; j++ ){
  //    Serial.print(j);
  //    Serial.print(":\t");
  //    Serial.println(calib_param[j]);
  //  }
  return calib_param;
}

void write_calib(char *write_string)
{

  char read_sensor;
  String read_string;
  int j = 0;

  nxp = flash_imu_calib.read();

  if (((String)write_string).length() != 0)
  {
    Serial.println("write_string");
    read_sensor = write_string[0];
    read_string = ((String)write_string).substring(1, ((String)write_string).length());
  }

  else
  {
    Serial.println("read from serial");
    read_sensor = Serial.read();
    read_string = Serial.readStringUntil('\r\n');
  }
  Serial.print("sensor:\t");
  Serial.println(read_sensor);

  Serial.print("string:\t");
  Serial.println(read_string);

  switch (read_sensor)
  {
  case 'a':
    read_string.toCharArray(nxp.accel_param, 100);
    break;

  case 'm':
    read_string.toCharArray(nxp.magneto_param, 100);
    break;

  case 'g':
    read_string.toCharArray(nxp.gyro_param, 100);
    break;
  }
  nxp.valid = true;
  // ...and finally save everything into "flash_imu_calib"
  flash_imu_calib.write(nxp);

  Serial.print(read_string);
  Serial.println("have been saved. Thank you!");
}

void print_calib_param()
{

  nxp = flash_imu_calib.read();

  Serial.print("accelerometer param:");
  Serial.println(nxp.accel_param);

  Serial.print("manetometer param:");
  Serial.println(nxp.magneto_param);

  Serial.print("gyroscope param:");
  Serial.println(nxp.gyro_param);
}

void calibrate_gyro()
{
  float gx, gy, gz, sum_gx = 0.0, sum_gy = 0.0, sum_gz = 0.0;
  sensors_event_t gevent;
  gyro.getEvent(&gevent);

  char calib_string[20];
  char str[30];

  for (int i = 0; i < num_of_ave_calib; i++)
  {
    gx = gevent.gyro.x;
    gy = gevent.gyro.y;
    gz = gevent.gyro.z;

    sum_gx = sum_gx + gx;
    sum_gy = sum_gy + gy;
    sum_gz = sum_gz + gz;
    Serial.println(i);
    delay(1);
  }

  sum_gx = sum_gx / num_of_ave_calib;
  sum_gy = sum_gy / num_of_ave_calib;
  sum_gz = sum_gz / num_of_ave_calib;

  Serial.println(sum_gx);
  Serial.println(sum_gy);
  Serial.println(sum_gz);

  strncpy(calib_string, "g", 1);
  sprintf(str, "%.2f", sum_gx);
  strncat(calib_string, str, String(str).length() + 1);
  strncat(calib_string, ";", 1);
  sprintf(str, "%.2f", sum_gy);
  strncat(calib_string, str, String(str).length() + 1);
  strncat(calib_string, ";", 1);
  sprintf(str, "%.2f", sum_gz);
  strncat(calib_string, str, String(str).length());

  //  Serial.println();
  write_calib(calib_string);
}