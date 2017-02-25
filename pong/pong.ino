/*
  This is a pong game using GLCD84x48 library.
  Created by Benjamin B., June 2, 2014.
  Released into the public domain.
*/

#include <GLCD84x48.h>
#define LCD_X 84
#define LCD_Y 48

GLCD84x48 lcd;

//inputs and outputs
const char speaker = 13; //speaker
const char button = 2;//button
const int rightA = A4, leftA = A5;//right and left rocker on PSX1

char buttonState = LOW;
char buttonD=0;//button debounce time
char soundDur = 0;//sound duration
//paddle
char lp_y = 20, rp_y = 20;//paddle coordinates
char p_rp_y = 20,  p_lp_y = 20;//previous paddle coordinates
const char p_h = 8, p_w = 3;//height and width of paddle
char lp_moves=0, rp_moves=0;//paddle moves
char paddle_speed=4;
//ball
char ball_x=41, ball_y=22;//ball coordiinate
char p_ball_x=41, p_ball_y=22;//previous ball coordinates
int ball_angle=0;
char ball_speed=2;
char ball_pass = 0;//ball passed paddle
char ballD = 3;//ball diameter
//info and gameplay
char wait=30;//pause
char l_points='0', r_points='0';
char diff = 80;//difficult
char complete=false;//game complete
char diffC='E';

const int angles[17] =  {0,  30,  45,  60,  60,  120,  135,  150, 180, -150, -135, -120, -120, -60, -45, -30, 0};
const char ang_x[16] =          {1,   1,   1,   2,   2,   -2,   -1,   -1,  -1,   -1,   -1,   -2,   -2,   2,   1,   1};
const char ang_y[16] =          {0,  -2,  -1,  -1,  -1,   -1,   -1,   -2,   0,    2,    1,    1,    1,   1,   1,   2};

char paddleSpeed(int a_val)
{
	if(a_val < 400)
	{
		if(a_val < 0)
			a_val = 0;
		
		a_val = 400 - a_val;
		return a_val/(400/paddle_speed); 
	}
	else if(a_val > 600)
	{
		if(a_val > 1000)
			a_val = 1000;
		
		a_val -= 600;
		return -a_val/(400/paddle_speed);
	}
	return 0;
}

void movePaddles()
{
	rp_moves=0;
	lp_moves=0;
	
	char speed = paddleSpeed(analogRead(rightA));
	if(speed > 0)
	{
		rp_moves=-1;
		lp_y = (lp_y < LCD_Y-p_h-speed)? lp_y+speed : LCD_Y-p_h-1;
	}	
	if(speed < 0)
	{
		rp_moves=1;
		lp_y = (lp_y > -speed)? lp_y+speed : 1;
	}
	
	speed = paddleSpeed(analogRead(leftA));
	if(speed > 0)
	{
		lp_moves=-1;
		rp_y = (rp_y < LCD_Y-p_h-speed)? rp_y+speed : LCD_Y-p_h-1;
	}	
	if(speed < 0)
	{
		lp_moves=1;
		rp_y = (rp_y > -speed)? rp_y+speed : 1;
	}
	
    drawPaddles();  
}

void drawPaddles()
{
	lcd.fRect(1, p_lp_y, p_w, p_h, 0);
	lcd.fRect(1, rp_y, p_w, p_h, 1);	
	lcd.fRect(LCD_X - p_w - 1, p_rp_y, p_w, p_h, 0);
	lcd.fRect(LCD_X - p_w - 1, lp_y, p_w, p_h, 1);
	p_lp_y = rp_y;
    p_rp_y = lp_y;	
}

void drawBall()
{
	lcd.fRect(p_ball_x, p_ball_y, ballD, ballD, 0);
	lcd.fRect(ball_x, ball_y, ballD, ballD, 1);
    p_ball_x=ball_x;
    p_ball_y=ball_y;
}

void delResults()
{
	lcd.fRect(52, 24, 5, 8, 0);
	lcd.fRect(32, 24, 5, 8, 0);
}

int reduceAngle(int a)
{
    if(a > 180)
        return a - 360;
    
    if(a < -150)
        return a + 360;
        
    return a;
}

int nextAngle(int a, char v_m)
{
	for(char i = 0; i < 16; i++)
		if(angles[i]==a)
			return reduceAngle(v_m*angles[i+1]);
}

void centerBall()
{
	ball_x=41; ball_y=22;
}

void newS(int s)
{
	ball_speed=2;
    wait=30;
	centerBall();
	ball_angle = s;
}

void resetP()
{
	l_points='0';
	r_points='0';
}

void beep()
{
	digitalWrite(speaker, HIGH);
	soundDur = 1;
}

void impact()
{
    if(ball_x!=p_w+1 && ball_x!=LCD_X-ballD-p_w-1 && ball_y!=1 && ball_y!=LCD_Y-ballD-1)//no impact
		return;
	
    if(ball_y==1)
    {
        if(((ball_x==p_w+1) && rp_y<=4) || ((ball_x==(LCD_X-p_w-ballD-1)) && lp_y<=4))
        {
			ball_angle = (ball_x==p_w+1)? -30 : -150;
			beep();
        }
        else if(ball_angle!=0 && ball_angle!=180)
        {
			ball_angle = -ball_angle;
			beep();
        }        
    }
    else if(ball_y==(LCD_Y-ballD-1))
    {
        if(((ball_x==p_w+1) && (rp_y>=LCD_Y-ballD-p_h-1)) || ((ball_x==(LCD_X-p_w-ballD-1)) && (lp_y>=LCD_Y-ballD-p_h-1)))
        {
			ball_angle = (ball_x==p_w+1)? 30 : 150;
			beep();
        }
        else if(ball_angle!=0 && ball_angle!=180)
        {
			ball_angle = -ball_angle;
			beep();
        }
    }
    else if(ball_x==p_w+1 && ((ball_y+ballD >= rp_y) && (ball_y <= rp_y+8)))
    {
        if(lp_moves == 0)
        {
			ball_angle = 180 - ball_angle;
        }
        else if(lp_moves == 1)
        {
			ball_angle = nextAngle(reduceAngle(180 - ball_angle), 1);
        }
        else if(lp_moves == -1)
        {
			ball_angle = nextAngle(reduceAngle(ball_angle - 180), -1);
        }
        beep();
        lp_moves = 0;
    }
    else if((ball_x==(LCD_X-p_w-ballD-1)) && ((ball_y+ballD >= lp_y) && (ball_y <= lp_y+8)))
    {
        if(rp_moves == 0)
        {
			ball_angle = 180 - ball_angle;
        }
        else if(rp_moves == 1)
        {
			ball_angle = nextAngle(reduceAngle(ball_angle - 180), -1);
        }
        else if(rp_moves == -1)
        {
			ball_angle = nextAngle(reduceAngle(180 - ball_angle), 1);
        }
        beep();
        rp_moves = 0;
    }
	ball_angle = reduceAngle(ball_angle);
}

char canDrawBallOnXY(char x, char y)
{	
	if(  (ball_x>=p_w+1 && ball_x<=LCD_X-p_w-ballD-1 && (y<1 || y>LCD_Y-ballD-1))  ||
		 (x>LCD_X-ballD-p_w-1 && y+ballD>=lp_y && y<=lp_y+p_h)  ||
	     (x<p_w+1 && y+ballD>=rp_y && y<=rp_y+p_h)  ||
		 (x<-2 || x>LCD_X-1)  )
		 return 0;
	
	return 1;
}

void moveBall()
{
   impact();
   if(wait>0)
   {
	   wait--;
	   return;
   }
   
   if(complete==true)
   {
	   delResults();
       complete=false;
   }
   
   if(soundDur == 0)
	   digitalWrite(speaker, LOW);
   else
	   soundDur--;
   
   char ball_xn=ball_x, ball_yn=ball_y, i;
   
   for(i = 0; i < 16; i++)
	   if(angles[i]==ball_angle)
	   {
		   ball_xn += ball_speed/ang_x[i];
		   if(ang_y[i]!=0)
			   ball_yn += ball_speed/ang_y[i];
		   break;
	   }
   
   //if no collision
   if(  ((ball_xn<=LCD_X-ballD-p_w-1 && ball_xn>=p_w+1 && ball_yn>=1 && ball_yn<=LCD_Y-ballD-1) || ball_pass==1)   ||
	     ((ball_xn<p_w+1 && (ball_yn+ballD<rp_y || ball_yn>rp_y+p_h)) || (ball_xn>LCD_X-ballD-p_w-1 && (ball_yn+ballD<lp_y || ball_yn>lp_y+p_h)))  )
   {
	   ball_x=ball_xn;
	   ball_y=ball_yn;
   }
   else
   {
	   //collision
	   while(canDrawBallOnXY(ball_x, ball_y))
	   {
		   ball_x += 2/ang_x[i];
		   if(ang_y[i]!=0)
			   ball_y += 2/ang_y[i];
	   }	   
	   
	   if(ball_y < 1)
		   ball_y = 1;
	   else if(ball_y>LCD_Y-ballD-1)
		   ball_y = LCD_Y-ballD-1;
	   
	   if((ball_x==LCD_X-ballD-p_w-2 || ball_x==LCD_X-ballD-p_w) && (ball_y+ballD>=lp_y) && (ball_y<=lp_y+p_h))
		   ball_x = LCD_X-ballD-p_w-1;
	   else if((ball_x==p_w || ball_x==p_w+2) && (ball_y+ballD>=rp_y) && (ball_y<=rp_y+p_h))
		   ball_x = p_w+1;
   }
   if(ball_x < p_w+1 || ball_x > LCD_X-ballD-p_w-1)
	   ball_pass = 1;
   
   drawBall();
   
   if(ball_x<-1)
   {
	   newS(180);
	   r_points++;
	   ball_pass = 0;
	   redrawLeft();
   }
   else if(ball_x>82)
   {
	   newS(0);
	   l_points++;
	   ball_pass = 0;
	   redrawRight();
   }
}

void redrawLeft()
{
	lcd.fRect(0, 0, 1, LCD_Y, 1);
	lcd.fRect(0, 0, 6, 1, 1);
	lcd.fRect(0, LCD_Y-1, 6, 1, 1);
}

void redrawRight()
{
	lcd.fRect(LCD_X-1, 0, 1, LCD_Y, 1);
	lcd.fRect(78, 0, 6, 1, 1);
	lcd.fRect(78, LCD_Y-1, 6, 1, 1);
}

void readButton()
{
    if(buttonD>0)
    {
        buttonD--;
        return;
    }
    else
    {
        buttonState = digitalRead(button);
		buttonD = (buttonState==HIGH)? 7 : 0;
    }
}

void changeDifficult()
{
    if(buttonState==HIGH)
    {
        buttonState=LOW;
		if(diff == 80)
			diff = 50;
		else if(diff == 50)
			diff = 1;
		else
			diff = 80;		
		
		newS(0);
        resetP();        
        complete=true;
		
		diffC='E';
        if(diff==50)
            diffC='M';
        else if(diff==1)
            diffC='H';
		
		lcd.printCharAtXY(52,24,diffC, 1);
		lcd.printCharAtXY(32,24,diffC, 1);
    }
}

void writePoints()
{
    lcd.printCharAtXY(35, 9, l_points, 1);
    lcd.printCharAtXY(45, 9, r_points, 1);
	for(char i=0; i<LCD_Y; i+=2)
    {
		lcd.setPixel(42, i, 1);
    }
}

void drawFence(void)
{
	lcd.eRect(0, 0, LCD_X, LCD_Y, 1, 1);
}

void completeGame()
{
    if(r_points>='7' || l_points>='7')
    {
        complete=true;
		newS(0);        
        
        if(r_points>='7')
        {
            lcd.printCharAtXY(52,24,'W', 1);
            lcd.printCharAtXY(32,24,'L', 1);
        }
        else
        {
            lcd.printCharAtXY(52,24,'L', 1);
            lcd.printCharAtXY(32,24,'W', 1);
        }
		resetP();
    }
}

void playAwesomePong()
{
	if(millis()%diff == 0)
	{
		movePaddles();
		writePoints();
		moveBall();
		readButton();
		changeDifficult();
		completeGame();
	}
}

void setup(void)
{
  pinMode(speaker, OUTPUT);
  pinMode(button, INPUT);
  lcd.lcdInitialise(4, 3, 5, 7, 6);
}
byte demo = 1;
void loop(void)
{
	if(demo)
	{
		byte r = 0;
		while(r<60)
			lcd.circle2(42, 24, r++, 1);
		
		while(r>=1)
			lcd.circle(42, 24, r--, 0);

			char aw[7] = {'A', 'W', 'E', 'S', 'O', 'M', 'E'};
		for(byte i = 0; i < 7*6; i+=6)
			lcd.printCharAtXY(20+i, 15, aw[i/6], 1);
		
		char po[4] = {'P', 'O', 'N', 'G'};
		for(byte i = 0; i < 4*6; i+=6)
			lcd.printCharAtXY(30+i, 25, po[i/6], 1);
		
		
		delay(5000);
		lcd.clearLcd();
		drawFence();
		demo = 0;
	}
    
	playAwesomePong();
}