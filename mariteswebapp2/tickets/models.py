from django.db import models

class MaintenanceTicketTriggers(models.Model):
    logger_fk_id = models.PositiveSmallIntegerField(primary_key=True)
    logger_name = models.CharField(max_length=7)
    cycle_2_status = models.CharField(max_length=20, blank=True, null=True)
    cycle_2_value = models.FloatField(blank=True, null=True)
    rain_info = models.PositiveSmallIntegerField(blank=True, null=True)
    data_presence = models.FloatField(blank=True, null=True)
    check_data = models.CharField(max_length=3, blank=True, null=True)

    class Meta:
        db_table = 'maintenance_ticket_triggers'

class MaintenanceTickets(models.Model):
    ticket_id = models.SmallAutoField(primary_key=True)
    logger_fk_id = models.PositiveSmallIntegerField()
    ticket_cycle_2_status = models.CharField(max_length=20)
    rain_state_history_fk_id = models.PositiveIntegerField()
    data_presence_percentage = models.PositiveIntegerField()
    ts_issued = models.DateTimeField()
    ts_maintenance_queue = models.DateTimeField(blank=True, null=True)
    ts_resolved = models.DateTimeField(blank=True, null=True)
    last_inbox_fk_id = models.PositiveIntegerField()
    resolver = models.SmallIntegerField(blank=True, null=True)
    validity_status = models.SmallIntegerField(blank=True, null=True)

    class Meta:
        db_table = 'maintenance_tickets'

class Loggers(models.Model):
    logger_id = models.SmallAutoField(primary_key=True)
    site = models.ForeignKey('Sites', models.DO_NOTHING)
    logger_name = models.CharField(max_length=7, blank=True, null=True)
    date_activated = models.DateField(blank=True, null=True)
    date_deactivated = models.DateField(blank=True, null=True)
    latitude = models.DecimalField(max_digits=9, decimal_places=6, blank=True, null=True)
    longitude = models.DecimalField(max_digits=9, decimal_places=6, blank=True, null=True)
    model = models.ForeignKey('LoggerModels', models.DO_NOTHING)

    class Meta:
        managed = False
        db_table = 'loggers'
        unique_together = (('logger_id', 'site', 'logger_name', 'model'),)

class Sites(models.Model):
    site_id = models.AutoField(primary_key=True)
    site_code = models.CharField(unique=True, max_length=3)
    purok = models.CharField(max_length=45, blank=True, null=True)
    sitio = models.CharField(max_length=45, blank=True, null=True)
    barangay = models.CharField(max_length=45, blank=True, null=True)
    municipality = models.CharField(max_length=45, blank=True, null=True)
    province = models.CharField(max_length=45, blank=True, null=True)
    region = models.CharField(max_length=45, blank=True, null=True)
    psgc = models.PositiveIntegerField(blank=True, null=True)
    active = models.IntegerField()
    households = models.CharField(max_length=255, blank=True, null=True)
    season = models.IntegerField()
    area_code = models.IntegerField()
    latitude = models.DecimalField(max_digits=9, decimal_places=6)
    longitude = models.DecimalField(max_digits=9, decimal_places=6)
    has_cbewsl = models.IntegerField()
    is_mlgu_handled = models.IntegerField()
    has_groupchat = models.IntegerField()

    class Meta:
        managed = False
        db_table = 'sites'

class LoggerModels(models.Model):
    model_id = models.AutoField(primary_key=True)
    has_tilt = models.IntegerField(blank=True, null=True)
    has_rain = models.IntegerField(blank=True, null=True)
    has_piezo = models.IntegerField(blank=True, null=True)
    has_soms = models.IntegerField(blank=True, null=True)
    has_gnss = models.IntegerField(blank=True, null=True)
    has_stilt = models.IntegerField(blank=True, null=True)
    logger_type = models.CharField(max_length=10, blank=True, null=True)
    version = models.IntegerField(blank=True, null=True)

    class Meta:
        managed = False
        db_table = 'logger_models'
