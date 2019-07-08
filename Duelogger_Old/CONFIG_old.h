//for EEET
//id = id - 0x04;

//for Gamut Berm
if (id < 0x07){
		id = id - 0x05;
}
else{
		id = id - 0x07;
}

//for Gamut Toe
if (id < 0x15){
		id = id - 0x05;
}	
else {
		id = id - 0x07;	
}

// for Oslaoo Berm
if (id < 0x05){
		id = id - 0x03;
}
else {
		id = id - 0x05;
}

// for Oslao Toe
id = id - 0x0F;

// for Sinipsip Berm

if ((id == 7) || (id == 8)){
		xh = ~xh;	xl = ~xl;	yh = ~yh;
		yl = ~yl;	zh = ~zh;	zl = ~zl;
}
if (id < 0x09){
		id = id - 0x01;
}	
else {
		id = id - 0x03;	
}

// for puguis Berm

if (id > 0x0C){
		id = id - 0x02;
}	

// for sinipsip Toe
id = id - 0x01;

// for Lipanto TOE
id = id - 0x09;

// for Lipanto Berm
id = id - 0x04;

// for Sinipsip Toe 2
id = id - 0x01;

// for BLCB
if (id < 0x19){
		id = id - 0x07;
}	
else {
		id = id - 0x09;	
}

// for BLCT
if (id < 0x11){
		id = id - 0x02;
}	
else {
		id = id - 0x04;	
}

//for HUMT
id = id - 0x04;