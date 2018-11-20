/**
Visualize a cube which will assumes the orientation described
in a quaternion coming from the serial port.

INSTRUCTIONS: 
This program has to be run when you have the FreeIMU_serial
program running on your Arduino and the Arduino connected to your PC.
Remember to set the serialPort variable below to point to the name the
Arduino serial port has in your system. You can get the port using the
Arduino IDE from Tools->Serial Port: the selected entry is what you have
to use as serialPort variable.


Copyright (C) 2011-2012 Fabio Varesano - http://www.varesano.net/

This program is free software: you can redistribute it and/or modify
it under the terms of the version 3 GNU General Public License as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

2-7-14 Program severely modified by Michael J Smorto, same license and warranty
applies.

*/

import processing.serial.*;
import processing.opengl.*;

import peasy.*;
PeasyCam cam, cam2;

import controlP5.*;
ControlP5 cp5;

import g4p_controls.*;
GButton btnStart;
GButton btnExit;

PrintWriter output;

Serial myPort;  // Create object from Serial class

final String serialPort = "COM5"; // replace this with your serial port. On windows you will need something like "COM1".
int BaudRate=57600;
String File_Name = "IMU-Cuire-MagDefTest.txt";
int HAS_GPS = 0;

final int burst = 5096;
int send_burst = (int) (0.75*burst);

//setting a 1D Kalman filter
//uncomment if you have removed complimentary altitude filter from library
//MyKalman pressK = new MyKalman();

//Settup Stop Watch
StopWatchTimer sw = new StopWatchTimer();

    //screen limits in lat, lon and elevation
    float west = -73.81465;
    float east = -79.81455;
    float north = 40.77459;
    float south = 40.77450;
    float lowest = 50;
    float highest = 200;

    //screen display point coordinates
    float screen_X;
    float screen_Y;
    float screen_Z;

// These are needed for the moving average calculation
float[] data = new float[32];
float total = 0, average = 0;
int p = 0, n = 0;

//LPF
float filterFactor = 0.05;
float heading_f = 0.;

//Moving average Heading
float corr_heading;
float heading_avg;
float heading = 0;
float oldHeading = 0.0;
int windSize = 96;

//set motiondetect types
float motionDetect;

float [] q = new float [4];
float [] acc = new float [3];
float [] gyro = new float [3];
float [] magn = new float [3];
float [] ypr = new float [3];
float temp; float press; float altitude; 
float EstAlt;
float dt;
float tnew;
float told = 0;

// Altitude - Accel Complimentary filter setup
float[] dyn_acc = new float[3];
float fused_alt;

// GPS Variables
float hdop, lat, longt, cog, sog, gpsalt, gpschars;
float hdop_val, loc_val, gpsalt_val, sog_val, cog_val;

// Instrument settings
float size = 38.0;
int tabHeight=80; // Extra height needed for Tabs

int xGraph      = 100;        int yGraph      = 180+tabHeight;
int xObj        = 520;        int yObj        = 293+tabHeight;
int xCompass    = 920;        int yCompass    = 341+tabHeight;
int xLevelObj   = 920;        int yLevelObj   = 80+tabHeight; 
int xGPS        = 853;        int yGPS        = 438+tabHeight;
final int VIEW_SIZE_X = 1000, VIEW_SIZE_Y = 600;

int horizonInstrSize, GPS_distanceToHome, GPS_directionToHome,
    GPS_numSat, GPS_fix, GPS_update, GPS_altitude, GPS_speed,
    GPS_latitude, GPS_longitude, graph_on;
  
int time, time4;
    
int i, j;

cGraph g_graph;

float angCalc, pVersion, a, b, h;

// Rotation Variables
float camDistance = -90;
float rotationX = 100;
float rotationY = 170;
float velocityX = 0;
float velocityY = 0;

// Texture
PImage texmap, Aimage, cmpRng, cmpAP;
PImage Top, Bottom, Right, Left, Front, Back;
float angx, angy, angz, angyLevelControl;

float S;
float A;

float sea_press = 1013.25;           //Input local sea level pressure
String seapresscmd = "99";
//float declinationAngle = -13.1603;   //Flushing, NY magnetic declination in degrees
float declinationAngle = 0;
float STATIONALTFT = 36.;           //LaGuardia AP measurement height
float SEA_PRESS  = 1013.25;          //default sea level pressure level in mb
float KNOWNALT   = 65.0;             //default known altitude, 
float INHG       = 0.02952998751;    //convert mb to in/Hg constant
float MB         = 33.8638815;       //convert in/Hg to mb constant
float FTMETERS   = 0.3048;
float METERS2FT  = 3.2808399;
float PI         = 3.14159;
float deg2rads   = PI/180;
float rad2degs    = 180/PI;

//flags
int calib = 0;             // Turn calibration on or off
int ArtHorFlg = 0;         // Make artificial horizion visible or not
int drawGraph = 0;
int PrintOutput = 0;       // Output raw data to standard file name
// Switch for ODO
int cube_odo = 0;          // Execute ODO routine

//FreeIMY setup
float [] hq = null;
float [] Euler = new float [3]; // psi, theta, phi

int lf = 10; // 10 is '\n' in ASCII
byte[] inBuffer = new byte[22]; // this is the number of chars on each line from the Arduino (including /r/n)

//---------------------------------------------------
// types needed for ODO implementation
//
float Sample_X;
float Sample_Y; 
float Sample_Z; 
long [] Sensor_Data = new long [8]; 
short countx; short county ;

float [] accelerationx = new float [2];
float [] accelerationy = new float [2];
float [] accelerationz = new float [3];
float [] velocityx = new float [2];
float [] velocityy = new float [2];
float [] velocityz = new float [2];
float [] positionX = new float [2]; 
float [] positionY = new float [2]; 
float [] positionZ = new float [2]; 
float dts;
float statex,statey, statez;
float statex_avg, statey_avg,statez_avg;
float motionDetect_transition, motionDetect_old;
int state_cnt;
float tempxxx = 0;
  
//-------------------------------------
//
PFont font, font8, font9, font10, font12, font15, letters, lcd;

int count = 0;

String skpath;

void myDelay(int time) {
  try {
    Thread.sleep(time);
  } catch (InterruptedException e) { }
}

//void settings() {
//  size(1000, 650, P3D);
//  smooth(4);
//}

//////////////////////////////////////////////////////////////
void setup() 
{
  size(1000, 620, OPENGL);

  skpath = sketchPath("") + "/";

  // Create a new file in the sketch directory
  output = createWriter(File_Name); 
  
  // The font must be located in the sketch's "data" directory to load successfully
  font = loadFont("CourierNew36.vlw"); 
  lcd = loadFont("LiquidCrystal-128.vlw");
  font8 = createFont("Arial bold",8,false);
  font9 = createFont("Arial bold",9,false);
  font10 = createFont("Arial bold",10,false);
  font12 = createFont("Arial bold",12,false);
  font15 = createFont("Arial bold",15,false);

  cp5 = new ControlP5(this);
  PFont pfont = createFont("Arial Bold",36,true); // use true/false for smooth/no-smooth

  ControlFont font1 = new ControlFont(pfont, 14);
  //sets up value fields for startup options
  //cp5.setControlFont(font,14);
  //cp5.setColorValue(color(255, 255, 0));
  //cp5.setColorLabel(color(255, 255, 0));
  setValues();

  //add button to open rolling trace in gwoptics
  //cp5.addButton("GraphWin")
  //   .setPosition(50,420)
  //   .setSize(240,30)
  //   .setCaptionLabel("Open Rolling Trace Frame")
  //   ;
  //btnStart = new GButton(this, 50, 420, 240, 30, "Open Rolling Trace Frame");
  btnExit = new GButton(this, 750, 520, 70, 50, "Exit");

  cp5.addTab("setup")
     .setColorBackground(color(0, 160, 100))
     .setColorLabel(color(255))
     .setColorActive(color(255,128,0))
     ;
  cp5.getTab("default")
     .setLabel("FreeIMU_GUI")
     .setId(1)
     ;

     
  //add button to open gps trace window
  //cp5.addButton("GPStrace")
  //   .setPosition(500,420)
  //   .setSize(240,30)
  //   .setCaptionLabel("Open Rolling Trace Frame")
  //   ;

  graphSetup();  
  //load cube images
  textureMode(NORMAL);
  Top = loadImage("Top.png");
  Bottom = loadImage("Bottom.png");
  
  //serial port set up
  myPort = new Serial(this, serialPort, BaudRate);

  //elapsed time start call
  sw.start();
  
  println("Waiting IMU..");

  myDelay(2000);

  while (myPort.available() != 0) {
    println(myPort.available());
      myPort.write("v");
    while ( myPort.readString() == null) {
      //myPort.write("1");
      myDelay(1000);
    }
  }

  cp5.setAutoDraw(false);


}

///////////////////////////////////////////////////////////////////
void draw() {
  
  //background(#585858);
  background(#000000);
  textFont(font, 18);
  textAlign(LEFT, TOP);
  strokeWeight(3);
  fill(#ffffff);
  
  cp5.draw();
  
  if(hq != null) { // use home quaternion
    quaternionToEuler(quatProd(hq, q), Euler);
    text("Disable home position by pressing \"n\"", 20, VIEW_SIZE_Y - 30);
 }
  else {
    quaternionToEuler(q, Euler);
    text("Point FreeIMU's X axis to your monitor then press \"h\"", 20, VIEW_SIZE_Y - 30);
  }

  fill(#FFFF00);
  fused_alt = altitude + STATIONALTFT/METERS2FT;
  text("Temp: " + temp + "\n" + "Press: " + press + "\n" , 20, VIEW_SIZE_Y - 70);
  textFont(font,18);
  text("ALT:",(VIEW_SIZE_X/2)-75,20); 
  fill(#00CF00);
  textFont(font,20);
  text(nfp((fused_alt),3,2),(VIEW_SIZE_X/2)-20,20); 
  textFont(font,18);
  fill(#ffff00);
  text("DeltaT: " + dt, 180, VIEW_SIZE_Y - 70);
  
  text("Q:\n" + q[0] + "\n" + q[1] + "\n" + q[2] + "\n" + q[3], 20, 20);

  //textFont(font, 18);
  //text("Pitch:\n", xLevelObj-170, yLevelObj - 0);
  //text("Roll:\n", xLevelObj-170, yLevelObj + 50);
  text("Yaw:\n", xLevelObj-60, yLevelObj + 165);
  //text("Heading:\n",850,255);
  
  //text(nfp(degrees(Euler[1]),3,2) + "  " + nfp(degrees(Euler[2]),3,2), xLevelObj + 85, yLevelObj + 125);
  getYawPitchRollRad();
  
  fill(#00CF00);
  //xtext(nfp(degrees(Euler[1]),3,2), xLevelObj-40, yLevelObj + 75);
  //xtext(nfp(degrees(Euler[2]),3,2), xLevelObj-40, yLevelObj + 135);
  //xtext(nfp(degrees(Euler[0]),3,2), xLevelObj-40, yLevelObj + 195);
  //text(nfp(degrees(ypr[1]),3,2), xLevelObj-150, yLevelObj + 25);
  //text(nfp(degrees(ypr[2]),3,2), xLevelObj-150, yLevelObj + 75);
  text(nfp(degrees(ypr[0]),3,2), xLevelObj-10, yLevelObj + 165);

  if(HAS_GPS == 1){
    fill(#ffff00);    
    text("Latitude:\n", xLevelObj-190, yLevelObj - 55); // was 50, 195
    text("Long:\n", xLevelObj-190, yLevelObj - 0);
    text("CoG:\n", xLevelObj-190, yLevelObj + 55);
    text("SoG:\n", xLevelObj-190, yLevelObj + 110);
    //text("GPS Alt:\n", xLevelObj-190, yLevelObj + 165);  
    text("GPS Alt:\n", (VIEW_SIZE_X/2)-115,50); 
    
    if(motionDetect == 0) {
      sog = 0;
      cog = -9999; }
    
    fill(#00CF00);
    text(nfp(lat,3,5), xLevelObj-190, yLevelObj - 30);  //was 160, 220
    text(nfp(longt,3,5), xLevelObj-190, yLevelObj + 25);
    text(nfp(cog,3,2), xLevelObj-190, yLevelObj + 80);
    text(nfp(sog,3,2), xLevelObj-190, yLevelObj + 135);
    //text(nfp(gpsalt,3,2), xLevelObj-190, yLevelObj + 190);
    text(nfp(gpsalt,3,2), (VIEW_SIZE_X/2)-20,50);
  }
  
  textFont(font, 18);
  fill(#FFFF00);
  noFill();
  stroke(204, 102, 0);
  rect(10, 17, 145, 95, 7);
  //angx = Euler[2];
  //angy = Euler[1];
  angx = ypr[2];
  angy = ypr[1];
  
  //heading = degrees(ypr[0]);
  //if( heading < 0 ) heading += 360.0; // convert negative to positive angles
  //Compass averaging
  //currentAngle = myAtan2(mouseY-height/2, mouseX-width/2) + radians(myNoise); 
  addItemsToHistoryBuffers(radians(heading));
  calculateMathematicalAverageOfHistory();
  calculateYamartinoAverageOfHistory(); 
  
  //corr_heading = heading;
  corr_heading = degrees(yamartinoAverageAngle);
  //rotComp();
  
  //textFont(font, 24);
  //fill(#00CF00);
  //text(nfp(((corr_heading)),4,1),850,285);
  //textFont(font, 18);
  //fill(#FFFF00);
  
  noFill();
  stroke(204, 102, 0);
  rect(10, 125, 145, 85, 7);  
  text("Acc:\n" + nfp(acc[0],1,6) + "\n" + nfp(acc[1],1,6) + "\n" + nfp(acc[2],1,6) + "\n", 20, 130);
  //text("Dyn Acc:\n" + nfp(dyn_acc[0],1,6) + "\n" + nfp(dyn_acc[1],1,6) + "\n" + nfp(dyn_acc[2],1,6) + "\n", 20, 130);
  
  //rect(10, 210, 145, 85, 7); 
  //text("Gyro:\n" + nfp(gyro[0],1,6) + "\n" + nfp(gyro[1],1,6) + "\n" + nfp(gyro[2],1,6) + "\n", 20, 220);
  rect(170, 20, 145, 85, 7);
  text("Gyro:\n" + nfp(gyro[0],1,6) + "\n" + nfp(gyro[1],1,6) + "\n" + nfp(gyro[2],1,6) + "\n", 180, 25);
  
  //rect(10, 295, 145, 90, 7); 
  //text("Magn:\n" + nfp(magn[0],1,6) + "\n" + nfp(magn[1],1,6) + "\n" + nfp(magn[2],1,6) + "\n", 20, 310);
  rect(170, 125, 145, 85, 7); 
  text("Magn:\n" + nfp(magn[0],1,6) + "\n" + nfp(magn[1],1,6) + "\n" + nfp(magn[2],1,6) + "\n", 180, 130);
  
  //text(MotionDetect(),VIEW_SIZE_X-75,VIEW_SIZE_Y-75) ;

  if(motionDetect > 0 ){
    fill(#FF0000);
  } else {
    fill(#FFFFFF)
  ; }
  //rect(VIEW_SIZE_X-75,VIEW_SIZE_Y-55,35,35);
  rect(900,525,35,35);
  
  if(cube_odo == 1) { 
     position();
     text("px:  " + positionX[0] + "\n" + "py:  " + positionY[0] + "\n" + "pz:  " + positionZ[0], 20, 235);
  }
 
    if (graph_on==1) {
      time=millis();
      if ((time-time4)>40) {
        time4=time;
        accxData.addVal(acc[0]); accyData.addVal(acc[1]); acczData.addVal(acc[2]);
        gyroxData.addVal(gyro[0]); gyroyData.addVal(gyro[1]); gyrozData.addVal(gyro[2]);
        magxData.addVal(magn[0]); magyData.addVal(magn[1]); magzData.addVal(magn[2]);
        altData.addVal(altitude);
        yawData.addVal(ypr[0]*rad2degs);
        pitchData.addVal(ypr[1]*rad2degs);
        rollData.addVal(ypr[2]*rad2degs);
        //axSlider.setValue(acc[0]); aySlider.setValue(acc[1]); azSlider.setValue(acc[2]);
        //gxSlider.setValue(gyro[0]); gySlider.setValue(gyro[1]); gzSlider.setValue(gyro[2]);
        //altSlider.setValue(altitude); magxSlider.setValue(magn[0]);magySlider.setValue(magn[1]);magzSlider.setValue(magn[2]);
        //yawSlider.setValue(ypr[0]*rad2degs);pitchSlider.setValue(ypr[1]*rad2degs);rollSlider.setValue(ypr[2]*rad2degs);
      }
    }
  
  if(drawGraph == 1) graph();
  drawCube();
  magnetron();
  flci();
  compass();
  
  textFont(font,20);
  fill(#ffff00);
  text("Elapsed\n" + "Time", 680,40);   
  textFont(lcd, 48);
  fill(#ff0000);
  text(sw.hour() + ":" + sw.minute() + ":" + sw.second(), 823, 40);
  
  
  //position();
  //text("px:  " + positionX[0] + "\n" + "py:  " +positionY[0] + "\n" + "pz:  " + positionZ[0],(width/2) - 150, (height/2)-100);

  if(PrintOutput == 1){
      output.println(sw.hour() + "," + sw.minute() + "," + sw.second() + ","+
         acc[0]+","+acc[1]+","+acc[2]+","+gyro[0]+","+gyro[1]+","+gyro[2]+","+
         magn[0]+","+magn[1]+","+magn[2] + "," + temp + "," +
         dyn_acc[0]+","+dyn_acc[1]+","+dyn_acc[2]+","+
         dt+","+corr_heading+","+ypr[0]*rad2degs+","+ypr[1]*rad2degs+","+ ypr[2]*rad2degs +","
         +Euler[0]*rad2degs+","+Euler[1]*rad2degs+","+Euler[2]*rad2degs+","+
         motionDetect+","+motionDetect_transition+","+fused_alt+","+q[0]+","+q[1]+","+q[2]+","+q[3]+","+
         positionX[0]+","+positionY[0]+","+positionZ[0]+", "+lat+", "+longt+", "+gpsalt);
  }
}

////////////////////////////////////////////////////////////////////
//void mousePressed() {
  //rollingGraph.getSurface().setVisible(true);
//}
 
//static final void removeExitEvent(final PSurface surf) {
//  final java.awt.Window win
//    = ((processing.awt.PSurfaceAWT.SmoothCanvas) surf.getNative()).getFrame();
 
//  for (final java.awt.event.WindowListener evt : win.getWindowListeners())
//    win.removeWindowListener(evt);
//}
 
////////////////////////////////////////////////////////////////////
float decodeFloat(String inString) {
  byte [] inData = new byte[4];
  
  if(inString.length() == 8) {
    inData[0] = (byte) unhex(inString.substring(0, 2));
    inData[1] = (byte) unhex(inString.substring(2, 4));
    inData[2] = (byte) unhex(inString.substring(4, 6));
    inData[3] = (byte) unhex(inString.substring(6, 8));
  }
      
  int intbits = (inData[3] << 24) | ((inData[2] & 0xff) << 16) | ((inData[1] & 0xff) << 8) | (inData[0] & 0xff);
  return Float.intBitsToFloat(intbits);
}

////////////////////////////////////////////////////////////////////////
void serialEvent(Serial p) {
  if(p.available() >= 18) {
    String inputString = p.readStringUntil('\n'); 
    //print(inputString);
    if (inputString != null && inputString.length() > 0) {
      String [] inputStringArr = split(inputString, ",");
      if(inputStringArr.length >= 18) { // q1,q2,q3,q4,\r\n so we have 5 elements
        q[0] = decodeFloat(inputStringArr[0]);
        q[1] = decodeFloat(inputStringArr[1]);
        q[2] = decodeFloat(inputStringArr[2]);
        q[3] = decodeFloat(inputStringArr[3]);
        acc[0] = decodeFloat(inputStringArr[4]);
        acc[1] = decodeFloat(inputStringArr[5]);
        acc[2] = decodeFloat(inputStringArr[6]);
        gyro[0] = decodeFloat(inputStringArr[7]);
        gyro[1] = decodeFloat(inputStringArr[8]);
        gyro[2] = decodeFloat(inputStringArr[9]);
        magn[0] = decodeFloat(inputStringArr[10]);
        magn[1] = decodeFloat(inputStringArr[11]);		
        magn[2] = decodeFloat(inputStringArr[12]);
        temp = decodeFloat(inputStringArr[13]);
        press = decodeFloat(inputStringArr[14]);
        //dt = (1./decodeFloat(inputStringArr[15]))/4;
        dt = (1./decodeFloat(inputStringArr[15]));
        heading = decodeFloat(inputStringArr[16]);
        //dt = tnew - told;
        //told = tnew;
        if(heading < -9990) {
            heading = 0;
        }
        altitude = decodeFloat(inputStringArr[17]);
        motionDetect = decodeFloat(inputStringArr[18]);
        
      //read GPS
      if(HAS_GPS == 1){
          hdop = decodeFloat(inputStringArr[19]);
          hdop_val = decodeFloat(inputStringArr[20]);
          lat = decodeFloat(inputStringArr[21]);
          longt = decodeFloat(inputStringArr[22]);
          loc_val = decodeFloat(inputStringArr[23]);
          gpsalt = decodeFloat(inputStringArr[24]);
          gpsalt_val = decodeFloat(inputStringArr[25]);
          cog = decodeFloat(inputStringArr[26]);
          cog_val = decodeFloat(inputStringArr[27]);
          sog = decodeFloat(inputStringArr[28]);
          sog_val = decodeFloat(inputStringArr[29]);
          gpschars = decodeFloat(inputStringArr[30]);    
       }        
      }
    }
    
    count = count + 1;
    //if(count == send_burst) { // ask more data when burst completed
      //1 = RESET MPU-6050, 2 = RESET Q Matrix
      if(key == '2') {
         myPort.clear();
         myPort.write("2");
         sw.start();
         println("pressed 2");
         key = '0';
         p.write("z");
      } else if(key == 'r') {
            myPort.clear();
            myPort.write("1");
            sw.start();
            println("pressed 1");
            key = '0';
            p.write("z");
      } else if(key == 'g') {
            myPort.clear();
            myPort.write("g");
            myPort.clear();
            myPort.write("2");
            sw.start();
            println("pressed g");
            key = '0';
            p.write("z");
      } else if(key == 'R') {
            myPort.clear();
            ArtHorFlg = 0;
            calib = 1;
            sea_press = 1013.25;
            setup();
            p.write("z");
      } 
      
      if(seapresscmd != "99"){
         myPort.clear();
         myPort.write(seapresscmd);
         seapresscmd =  "99";
         p.write("z");
      }   
      
      if(calib == 0) {
         myPort.clear();
         myPort.write("f");
         sw.start();
         calib = 99;
         p.write("z");
      }    
      if(calib == 1) {
         myPort.clear();
         myPort.write("t");
         sw.start();
         calib = 99;
         p.write("z");
      }

      //myDelay(100);
      count = 0;
      //p.write("z");

    //}


  }
}

///////////////////////////////////////////////////////////////////////////////
void keyPressed() {
  if(key == 'h') {
    println("pressed h");
    // set hq the home quaternion as the quatnion conjugate coming from the sensor fusion
    hq = quatConjugate(q);
    sw.start();
  }
  else if(key == 'n') {
    println("pressed n");
    hq = null;
  }
  else if(key == 's') {
    println("pressed s"); 
    sw.start();
  }
  else if(key == 'p') {
    println("pressed p"); 
    positionX[0]=0;
    positionY[0]=0;
    positionZ[0]=0;    
  }
  else if(key == 'v') {
    println("pressed v"); 
    //you will need to change this path
    String path = sketchPath("") + "/Applications/WebcamViewer.exe";
    //open("C:/Users/CyberMerln/Documents/Processing/FreeIMU_cube_Odo_Exp/Applications/WebcamViewer.exe");   
    //open(path);
  }
  
  else if(key == 'q') {
    output.flush(); // Writes the remaining data to the file
    output.close(); // Finishes the file
    myPort.write("v");
    exit();
  }
}

/////////////////////////////////////////////////////////////////////////////
// See Sebastian O.H. Madwick report 
// "An efficient orientation filter for inertial and intertial/magnetic sensor arrays" Chapter 2 Quaternion representation
void quaternionToEuler(float [] q, float [] euler) {
  euler[0] = atan2(2 * q[1] * q[2] - 2 * q[0] * q[3], 2 * q[0]*q[0] + 2 * q[1] * q[1] - 1); // psi
  euler[1] = -asin(2 * q[1] * q[3] + 2 * q[0] * q[2]); // theta
  euler[2] = atan2(2 * q[2] * q[3] - 2 * q[0] * q[1], 2 * q[0] * q[0] + 2 * q[3] * q[3] - 1); // phi
}

////////////////////////////////////////////////////////////////
float [] quatProd(float [] a, float [] b) {
  float [] q = new float[4];
  
  q[0] = a[0] * b[0] - a[1] * b[1] - a[2] * b[2] - a[3] * b[3];
  q[1] = a[0] * b[1] + a[1] * b[0] + a[2] * b[3] - a[3] * b[2];
  q[2] = a[0] * b[2] - a[1] * b[3] + a[2] * b[0] + a[3] * b[1];
  q[3] = a[0] * b[3] + a[1] * b[2] - a[2] * b[1] + a[3] * b[0];
  
  return q;
}

// returns a quaternion from an axis angle representation
float [] quatAxisAngle(float [] axis, float angle) {
  float [] q = new float[4];
  
  float halfAngle = angle / 2.0;
  float sinHalfAngle = sin(halfAngle);
  q[0] = cos(halfAngle);
  q[1] = -axis[0] * sinHalfAngle;
  q[2] = -axis[1] * sinHalfAngle;
  q[3] = -axis[2] * sinHalfAngle;
  
  return q;
}

// return the quaternion conjugate of quat
float [] quatConjugate(float [] quat) {
  float [] conj = new float[4];
  
  conj[0] = quat[0];
  conj[1] = -quat[1];
  conj[2] = -quat[2];
  conj[3] = -quat[3];
  
  return conj;
}

/////////////////////////////////////////////////////////////////////////
void getYawPitchRollRad() {
  //float q[4]; // quaternion
  float gx, gy, gz; // estimated gravity direction
  
  gx = 2 * (q[1]*q[3] - q[0]*q[2]);
  gy = 2 * (q[0]*q[1] + q[2]*q[3]);
  gz = q[0]*q[0] - q[1]*q[1] - q[2]*q[2] + q[3]*q[3];
  
  ypr[0] = atan2(2 * q[1] * q[2] - 2 * q[0] * q[3], 2 * q[0]*q[0] + 2 * q[1] * q[1] - 1);
  ypr[1] = atan(gx / sqrt(gy*gy + gz*gz));
  ypr[2] = atan(gy / sqrt(gx*gx + gz*gz));
}

//=============================================================
// converted from Michael Shimniok Data Bus code
// http://mbed.org/users/shimniok/code/AVC_20110423/

float clamp360(float x) {
    while ((x) >= 360.0) (x) -= 360.0; 
    while ((x) < 0) (x) += 360.0; 
    return x;
}

//==============================================================
//
float HeadingAvgCorr(float newx, float oldx) {
    while ((newx + 180.0) < oldx) (newx) += 360.0;
    while ((newx - 180.0) > oldx) (newx) -= 360.0;
    while ((newx) == 360.0) (newx) = 0.0;
    return newx;
}


//=======================================
public float iround(float number, float decimal) {
  int ix;
  ix = round(number*pow(10, decimal));
  return float(ix)/pow(10, decimal);
}

//=======================================

void gravityCompensateDynAcc() {
  float[] g = new float[3];
  
  // get expected direction of gravity in the sensor frame
  g[0] = 2 * (q[1] * q[3] - q[0] * q[2]);
  g[1] = 2 * (q[0] * q[1] + q[2] * q[3]);
  g[2] = q[0] * q[0] - q[1] * q[1] - q[2] * q[2] + q[3] * q[3];
  
  // compensate accelerometer readings with the expected direction of gravity
  dyn_acc[0] = acc[0] - g[0];
  dyn_acc[1] = acc[1] - g[1];
  dyn_acc[2] = acc[2] - g[2];
}