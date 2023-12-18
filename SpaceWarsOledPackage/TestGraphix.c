// * File:   Space Wars.c
// * Author: Shanee_Gat_227367448 and Lidia_Polykov_207550096
// *
// * Created on March 5, 2023
// *
///*
// * File:   TestGraphix.c
// * Author: Shanee Gat_227367448 and Lidia Polykov
// *
// *

#include <stdlib.h>
#include <stdio.h>

#include "System/system.h"
#include "System/delay.h"
#include "oledDriver/oledC.h"
#include "oledDriver/oledC_colors.h"
#include "oledDriver/oledC_shapes.h"
#include "i2cDriver/i2c1_driver.h"
#include <time.h>

#define DEVICE_ID 0xE5
#define DEVICE_ID_ADDRESS 0
#define DEVICE_WRITE_ADDRESS 0x3A
#define DEVICE_READ_ADDRESS 0x3B
#define MEASUREMENT_MODE 0x08
#define DEVICE_PWR_CTL 0x2D

unsigned int count = 0;
int count_shapes = 0;
int start_flag = 0;
int accelerometer = 0;
int prev_x = 0;
uint16_t seed = 0;

typedef enum { LASER, REGULAR, DELETED } ShapeType;

//linked list structs**************************************************************
typedef struct Shape{
    ShapeType type;
    int x;
    int y;
    int length;
}Shape;

typedef struct Shapes{
    Shape item;
    struct Shapes* next;
}Shapes;


Shapes* shapes_head = NULL;
Shapes* lasers_head = NULL;


typedef enum { OK, NACK, ACK, BAD_ADDR, BAD_REG } I2C_Error;
I2C_Error i2cReadSlaveRegister(unsigned char regAdd, unsigned char * reg) {
    i2c1_driver_start();
    i2c1_driver_TXData(DEVICE_WRITE_ADDRESS);
    if (i2c1_driver_isNACK()) return BAD_ADDR;
    
    i2c1_driver_TXData(regAdd);
    if (i2c1_driver_isNACK()) return BAD_REG;
    
    i2c1_driver_restart();
    i2c1_driver_TXData(DEVICE_READ_ADDRESS);
    if (i2c1_driver_isNACK()) return BAD_ADDR;
    
    i2c1_driver_startRX();
    i2c1_driver_waitRX();
    *reg = i2c1_driver_getRXData();
    i2c1_driver_sendNACK();
    i2c1_driver_stop();
    return OK;
}

I2C_Error i2cWriteSlaveRegister(unsigned char regAdd, uint8_t data) {
    i2c1_driver_start();
    
    i2c1_driver_TXData(DEVICE_WRITE_ADDRESS);
    if (i2c1_driver_isNACK()) return BAD_ADDR;
    
    i2c1_driver_TXData(regAdd);
    if (i2c1_driver_isNACK()) return BAD_REG;
    
    i2c1_driver_TXData(data);
    if (i2c1_driver_isNACK()) return BAD_REG;
    
    i2c1_driver_stop();
  
    return OK;
}

void init(){
    //buttons
    TRISAbits.TRISA11 = 1;
    TRISAbits.TRISA12 = 1;
    
    //LED
    OC1CON2bits.SYNCSEL = 0x1F;
    OC1CON2bits.OCTRIG = 0;
    OC1CON1bits.OCTSEL = 0b111;
    OC1CON1bits.OCM = 0b110;
    OC1CON2bits.TRIGSTAT = 1;
    
    OC2CON2bits.SYNCSEL = 0x1F;
    OC2CON2bits.OCTRIG = 0;
    OC2CON1bits.OCTSEL = 0b111;
    OC2CON1bits.OCM = 0b110;
    OC2CON2bits.TRIGSTAT = 1;
    
    OC3CON2bits.SYNCSEL = 0x1F;
    OC3CON2bits.OCTRIG = 0;
    OC3CON1bits.OCTSEL = 0b111;
    OC3CON1bits.OCM = 0b110;
    OC3CON2bits.TRIGSTAT = 1;
 
    RPOR13bits.RP26R = 13;
    RPOR13bits.RP27R = 14;
    RPOR11bits.RP23R = 15;
    
}


//LED signaling how many shapes are in the game
void LED(){
    
    if(count_shapes < 1){
        OC1R = 0;
        OC2R = 0; 
        OC3R = 0;
    }
    
    if(count_shapes == 1 || count_shapes == 2){
        OC1R = 0;
        OC2R = 300; 
        OC3R = 0;
    }            
    if(count_shapes > 2 && count_shapes < 4){
        OC1R = 0;
        OC2R = 0;
        OC3R = 300; 
    }
    if(count_shapes >= 4){
        OC1R = 300;
        OC2R = 0;
        OC3R = 0;
    }    
}

void drawRocket(int x){
    x/=5;
    x = x>50? 50 : x;
    x = x<-50? -50 : x;
 
    oledC_DrawRectangle(43-prev_x, 90, 57-prev_x, 100, OLEDC_COLOR_BLACK);
    oledC_DrawRectangle(49-prev_x, 85, 51-prev_x, 90, OLEDC_COLOR_BLACK);
    oledC_DrawRectangle(43-x, 90, 57-x, 100, OLEDC_COLOR_AQUA);
    oledC_DrawRectangle(49-x, 85, 51-x, 90, OLEDC_COLOR_AQUA);
    prev_x = x;
}

void blackOutShape(Shape shape){
    drawRec(&shape, OLEDC_COLOR_BLACK);   
}

//check if a laser beam hit a shape
void checkCollisions(){
    Shapes* shapes_runner = shapes_head;
    Shapes* lasers_runner = lasers_head;
   
    int keepRunnning = 1;
    while(keepRunnning && shapes_runner){
        if(shapes_runner->item.type == DELETED){
             shapes_runner = shapes_runner->next;
             continue;
        }
        while(keepRunnning && lasers_runner){
            if (lasers_runner->item.type == DELETED){
                lasers_runner = lasers_runner->next;
                continue;
            }  
            if (lasers_runner->item.y > shapes_runner->item.y){
                break;
            }     
            if(lasers_runner->item.y >= (shapes_runner->item.y - 1) && 
               lasers_runner->item.y <= (shapes_runner->item.y + 1) &&
               lasers_runner->item.x-1 <= (shapes_runner->item.x + shapes_runner->item.length -1) && 
               lasers_runner->item.x+1 >= shapes_runner->item.x){
               blackOutShape(lasers_runner->item);
               blackOutShape(shapes_runner->item);
               lasers_runner->item.type = DELETED;
               shapes_runner->item.type = DELETED;          
               break;         
            }
            lasers_runner = lasers_runner->next;
        }
         shapes_runner = shapes_runner->next;
         lasers_runner = lasers_head;
    }
}

void drawRec(Shape* shape, uint16_t color){
    oledC_DrawRectangle(shape->x, shape->y,(shape->x + shape->length), (shape->y +2), color);
}

//clear shapes
void blackOutShapes(Shapes* shapes){
    while(shapes != NULL){ 
        if (shapes->item.type != DELETED)
            drawRec(&shapes->item, OLEDC_COLOR_BLACK);   
        //if shape hit the floor GAME OVER 
         shapes = shapes->next;
    }
}


void GameOver(){
    oledC_DrawString(18, 15, 3, 3, "Game", OLEDC_COLOR_GREEN);
    oledC_DrawString(18, 45, 3, 3, "Over", OLEDC_COLOR_GREEN);
}

//free linked lists
void cleanUp(){
    while(lasers_head != NULL){
        Shapes* tmp = lasers_head->next;
        free(lasers_head);
        lasers_head = tmp;
    }
    while(shapes_head != NULL){
        Shapes* tmp = shapes_head->next;
        free(shapes_head);
        shapes_head = tmp;
    }
    count_shapes = 0;
}


void resetGame(){ 
    PR1=50000;
    PR2 = 50000;
    PR3=50000;
    
    DELAY_milliseconds(20);
    oledC_clearScreen();
    cleanUp();
    GameOver();
    DELAY_milliseconds(1000);
    start_flag = 0; 
}

//shape reached bottom of screen
size_t isOver(Shape* shape) {
    return (shape->y+2 == 100);
}

//laser beam hit top of screen
size_t reachedTop(Shape* shape) {
    return (shape->y <= 1);
}


//draw shapes
void drawShapes(uint16_t color){
     
    Shapes* runner = shapes_head;
    while(runner != NULL){       
        if (runner->item.type != DELETED){
            runner->item.y++;
            drawRec(&runner->item,color);
        }        
        
        if (isOver(&runner->item)){
            resetGame();
            return;
        }
        runner = runner->next;
    }
    
    runner = lasers_head;
    while(runner != NULL){ 
        if (runner->item.type != DELETED) {
            runner->item.y--;
            if (reachedTop(&runner->item)){
                runner->item.type = DELETED;
            }
            else{
                 drawRec(&runner->item,OLEDC_COLOR_HOTPINK);
            }
        }
        runner = runner->next;
    }
    
    checkCollisions();
    
}

void introduction1(uint16_t text_color){
    oledC_DrawString(26, 20, 1, 1, "Shenkar", text_color);
    oledC_DrawString(35, 32, 1, 1, "Tech", text_color);
    oledC_DrawString(25, 44, 1, 1, "Presents", text_color);
    oledC_DrawString(30, 56, 1, 1, "......", text_color);
}
void introduction2(uint16_t text_color){
    oledC_DrawString(10, 8, 3, 3, "Space", text_color);
    oledC_DrawString(20, 32, 3, 3, "Wars", text_color);
    oledC_DrawRectangle(0, 64, 100, 65, OLEDC_COLOR_GREEN);
    oledC_DrawString(9,  70, 1, 1, "Press any key ", text_color);
    oledC_DrawString(25, 82, 1, 1, "to begin ", text_color);
}


//Interrupt - add laser beams from linked list
void __attribute__((__interrupt__)) _IOCInterrupt(void) {
    
    //oledC_clearScreen();
    if(start_flag){   
        
        if (lasers_head){
            Shapes* runner = lasers_head;
            while(runner->next != NULL){
                runner = runner->next;
            }
            runner->next = (Shapes*)malloc(sizeof(Shapes));
            runner = runner->next;
            seed = rand();
            runner->item.length = 1;
            int x = accelerometer/5 ;
            x = x>50? 50 : x;
            x = x<-50? -50 : x;
            runner->item.x = 50-x;
            runner->item.y = 80;
            runner->item.type = LASER;
            runner->next = NULL;   
        }
        else{
            lasers_head = (Shapes*)malloc(sizeof(Shapes));
            seed = rand();
            lasers_head->item.length = 1;
            int x = accelerometer/5 ;
            x = x>50? 50 : x;
            x = x<-50? -50 : x;
            lasers_head->item.x = 50-x;
            lasers_head->item.y = 80;
            lasers_head->item.type = LASER;
            lasers_head->next = NULL;     
        }
    }
    else{
        oledC_clearScreen();
        ++start_flag;     
    }
    IFS1bits.IOCIF = 0; //reset interrupt flag
}


//Timer - draw rocket*******************************************************
void __attribute__((interrupt, no_auto_psv))_T1Interrupt(void){
    if (start_flag){
        rand();rand();rand();
        uint8_t xx[2];
        i2cReadSlaveRegister(0x32, &xx[0]);  
        DELAY_milliseconds(2);
        i2cReadSlaveRegister(0x33, &xx[1]); 


        int cur_accelerometer =  xx[0] | ( xx[1]<< 8); 
        if (accelerometer != cur_accelerometer){
            drawRocket(accelerometer); 
            accelerometer = cur_accelerometer;
        }
    }
    IFS0bits.T1IF = 0; //reset timer interrupt 
}


//draw shapes and laser beams
void __attribute__((interrupt, no_auto_psv))_T2Interrupt(void){
    if (start_flag){
        ++count;
        srand(seed);
        if(count%10 == 0){
            if (shapes_head){
                Shapes* tmp = shapes_head;
                while(shapes_head->next != NULL){
                    shapes_head = shapes_head->next;
                }
                shapes_head->next = (Shapes*)malloc(sizeof(Shapes));
                shapes_head = shapes_head->next;
                seed = rand();
                shapes_head->item.length = (seed % 3) + 3;
                shapes_head->item.x = 2 + seed % (97 - shapes_head->item.length);
                shapes_head->item.y = -1;
                shapes_head->item.type = REGULAR;
                shapes_head->next = NULL;   
                shapes_head = tmp;
            }
            else{
                shapes_head = (Shapes*)malloc(sizeof(Shapes));
                seed = rand();
                shapes_head->item.length = (seed % 3) + 3;
                shapes_head->item.x = 2 + seed % (97-shapes_head->item.length);
                shapes_head->item.y = -1;
                shapes_head->item.type = REGULAR;
                shapes_head->next = NULL;     
            }
            ++count_shapes;
            LED();
        }

        if (shapes_head != NULL || lasers_head != NULL) {
                    blackOutShapes(shapes_head);
                blackOutShapes(lasers_head);
                DELAY_milliseconds(5);

            drawShapes(OLEDC_COLOR_HOTPINK);
        }
    }

    IFS0bits.T2IF = 0; //reset timer interrupt 
}

//delete shapes and lasers
void __attribute__((interrupt, no_auto_psv))_T3Interrupt(void){
    if (start_flag){
        Shapes* tmp = lasers_head;
        Shapes* before = lasers_head;
        int first = 1;
        while(tmp){
            if (tmp->item.type == DELETED){
                if (tmp == lasers_head){
                    lasers_head = lasers_head->next;
                    tmp->next = NULL;
                    free(tmp);
                    tmp = lasers_head;
                    before = tmp;
                }
                else if (tmp->next == NULL){
                    before->next = NULL;
                    free(tmp);
                    tmp = NULL;
                }
                else{
                    before->next = tmp->next;
                    tmp->next = NULL;
                    free(tmp);
                    tmp = before->next;
                }
            } else {
                if (!first){
                    tmp = tmp->next;
                    before = before->next;
                }
                else{
                    first = 0;
                    tmp = tmp->next;
                }   
            }           
        } 

        tmp = shapes_head;
        before = shapes_head;
        first = 1;
        while(tmp){
            if (tmp->item.type == DELETED){
                if (tmp == shapes_head){
                    shapes_head = shapes_head->next;
                    tmp->next = NULL;
                    free(tmp);
                    tmp = shapes_head;
                    before = tmp;
                }
                else if (tmp->next == NULL){
                    before->next = NULL;
                    free(tmp);
                    tmp = NULL;
                }
                else{
                    before->next = tmp->next;
                    tmp->next = NULL;
                    free(tmp);
                    tmp = before->next;
                }
            } else {
                if (!first){
                    tmp = tmp->next;
                    before = before->next;
                }
                else{
                    first = 0;
                    tmp = tmp->next;
                }   
            }           
        } 
    }
    IFS0bits.T3IF = 0; //reset timer interrupt 

}

void setupTimers(){
        //timer1 spaceship
    IEC0bits.T1IE = 1; //disable timer1 interrupt
    T1CONbits.TON = 1; // stop timer1
    T1CONbits.TSIDL = 1;  //continue running in idle
    T1CONbits.TCKPS = 0b11; //prescalar 256
    IFS0bits.T1IF = 0; //clear timer
    INTCON2bits.GIE = 1; // set to global interrupt 
    PR1 = 1000;
    
    //timer2 shapes
    IEC0bits.T2IE = 1; //disable timer2 interrupt
    T2CONbits.TON = 1; // stop timer1
    T2CONbits.TSIDL = 1;  //continue running in idle
    T2CONbits.TCKPS = 0b11; //prescalar 256
    IFS0bits.T2IF = 0; //clear timer
    INTCON2bits.GIE = 1; // set to global interrupt
    PR2 = 7500;
    
     //timer3 shapes
    IEC0bits.T3IE = 1; //disable timer3 interrupt
    T3CONbits.TON = 1; // stop timer1
    T3CONbits.TSIDL = 1;  //continue running in idle
    T3CONbits.TCKPS = 0b11; //prescalar 256
    IFS0bits.T3IF = 0; //clear timer
    INTCON2bits.GIE = 1; // set to global interrupt
    PR3 = 2000;
}

int main(void) {
    
    SYSTEM_Initialize();
    uint16_t background_color = OLEDC_COLOR_BLACK;
    uint16_t item_color = OLEDC_COLOR_HOTPINK;
    
    oledC_setBackground(background_color);
    oledC_clearScreen();
    //LED
    OC1RS = 1023;
    OC2RS = 1023;
    OC3RS = 1023;
    OC1R = 0;
    OC2R = 0; 
    OC3R = 0;
    
            
    i2c1_driver_driver_close();
    DELAY_milliseconds(500);
    i2c1_driver_driver_open();
    DELAY_milliseconds(500);
    
    unsigned char id = 0;
    //is correct id?
    i2cReadSlaveRegister(DEVICE_ID_ADDRESS, &id);
    if (id != DEVICE_ID)
        exit(-1);
    //save to register
    DELAY_milliseconds(1500);
    i2cWriteSlaveRegister(DEVICE_PWR_CTL, MEASUREMENT_MODE);

    
    while (1) {
//       Introduction start
        if(!start_flag){
            oledC_clearScreen();
            DELAY_milliseconds(30);
            introduction1(item_color);
            DELAY_milliseconds(3000);
            oledC_clearScreen();
            DELAY_milliseconds(30);
            introduction2(item_color);
            init();
                //interrupt
            IFS1bits.IOCIF = 0;
            IEC1bits.IOCIE = 1;
            PADCONbits.IOCON = 1;
            IOCNAbits.IOCNA11 = 1;
            IOCNAbits.IOCNA12 = 1;
            INTCON2bits.GIE = 1;
            setupTimers();
            while(!start_flag){ 
                DELAY_milliseconds(30);
            }
            oledC_clearScreen();
        }
        
        //let game load
        while(start_flag){
            DELAY_milliseconds(1000);
        }
        
    }
    

    i2c1_driver_driver_close();
    return 0;
}
