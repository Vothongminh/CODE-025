#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <mysql.h>
#define MAXTIMINGS	85
#define DHTPIN		4
#define light 14
#define fan 15
#define pump 18
#define soil 23
int soid_ss = 0;
unsigned long int timer = 0;
int dht11_dat[5] = { 0, 0, 0, 0, 0 };
 //.....................................
void read_dht11_dat()
{
	set:;
	uint8_t laststate	= HIGH;
	uint8_t counter		= 0;
	uint8_t j		= 0, i;
 
	dht11_dat[0] = dht11_dat[1] = dht11_dat[2] = dht11_dat[3] = dht11_dat[4] = 0;
 
	pinMode( DHTPIN, OUTPUT );
	digitalWrite( DHTPIN, LOW );
	delay( 18 );
	digitalWrite( DHTPIN, HIGH );
	delayMicroseconds( 40 );
	pinMode( DHTPIN, INPUT );
 
	for ( i = 0; i < MAXTIMINGS; i++ )
	{
		counter = 0;
		while ( digitalRead( DHTPIN ) == laststate )
		{
			counter++;
			delayMicroseconds( 1 );
			if ( counter == 255 )
			{
				break;
			}
		}
		laststate = digitalRead( DHTPIN );
 
		if ( counter == 255 )
			break;
 
		if ( (i >= 4) && (i % 2 == 0) )
		{
			dht11_dat[j / 8] <<= 1;
			if ( counter > 16 )
				dht11_dat[j / 8] |= 1;
			j++;
		}
	}
 
	if ( (j >= 40) &&
	     (dht11_dat[4] == ( (dht11_dat[0] + dht11_dat[1] + dht11_dat[2] + dht11_dat[3]) & 0xFF) ) )
	{
		printf( "Humidity = %d.%d %% Temperature = %d.%d C\n",
			dht11_dat[0], dht11_dat[1], dht11_dat[2], dht11_dat[3]);
	}else  {
		//printf( "Data not good, skip\n" );
		goto set;
	}
}
 //.....................................
int main( void )
{
	MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    char *server = "localhost";
    char *user = "khoa";
    char *password = "123456"; /* set me first */
    char *database = "smart_garden";

	wiringPiSetupGpio();
    pinMode(light, OUTPUT);
    pinMode(fan, OUTPUT);
    pinMode(pump, OUTPUT);
	pinMode(soil, INPUT);
    digitalWrite(light, HIGH); //off
    digitalWrite(fan, HIGH);
    digitalWrite(pump, HIGH);
 
	if ( wiringPiSetup() == -1 ){
		exit( 1 );
	}
 
	while ( 1 )
	{
		// ket noi database
        conn = mysql_init(NULL);
        mysql_real_connect(conn,server,user,password,database,0,NULL,0); 
        // kiem tra cot isUpdated
        char sql[300];
        sprintf(sql, "select * from data where Num=(select max(Num) from data)");
        mysql_query(conn,sql);
        res = mysql_store_result(conn); 
        row = mysql_fetch_row(res); //row[0]-> xxx; row[1]->xxx
        // // bat tat relay
        if(atoi(row[7])==0){ //auto = 0
            if(atoi(row[4])==1){
				digitalWrite(light, LOW);
			} else {
				digitalWrite(light, HIGH);
			}
			if(atoi(row[5])==1){
				digitalWrite(fan, LOW);
			} else {
				digitalWrite(fan, HIGH);
			}
			if(atoi(row[6])==1){
				digitalWrite(pump, LOW);
			} else {
				digitalWrite(pump, HIGH);
			}
        }
		else if (atoi(row[7])==1){
			if((soid_ss == 0)||(dht11_dat[2] > 30)){
				digitalWrite(pump, LOW);
				digitalWrite(fan, LOW);
			} else if((soid_ss == 1)&&(dht11_dat[2] < 30)){
				digitalWrite(pump, HIGH);
				digitalWrite(fan, HIGH);
			}
			
		}
		//read sensor
		read_dht11_dat();
		delay(500);
		if(digitalRead(soil) == HIGH){soid_ss = 1;}
		else{soid_ss = 0;}
		//update db
		while((millis() - timer)>5000){
			sprintf(sql, "insert into data(Temp, Humd, Soil, Light, Fan, Pump, Auto, isUpdated) values (%d.%d, %d.%d, %d, %d, %d, %d, %d, %d)", dht11_dat[2], dht11_dat[3], dht11_dat[0], dht11_dat[1], 1, atoi(row[4]), atoi(row[5]), atoi(row[6]), atoi(row[7]), 1);
			mysql_query(conn,sql);
			timer = millis();
		}
		//dk cho status
		if ((dht11_dat[2] > 40)&&(dht11_dat[0]<30))
		{
			sprintf(sql, "update data set Status = 'Hot' where Num=(select max(Num) from data)");
			mysql_query(conn,sql);
		}
		else if ((dht11_dat[2] < 40)&&(dht11_dat[0] > 20)){
			sprintf(sql, "update data set Status = 'Normal' where Num=(select max(Num) from data)");
			mysql_query(conn,sql);
		}
		else if((dht11_dat[2] < 20)&&(dht11_dat[0] > 90)){
			sprintf(sql, "update data set Status = 'Wet' where Num=(select max(Num) from data)");
			mysql_query(conn,sql);
		}
	}
	return(0);
}