#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>


//Fixed values found in the document
#define REGISTRATION_SIZE 10
#define RESTROOM_SIZE 10
#define CAFE_NUMBER 10
#define GP_NUMBER 10
#define PHARMACY_NUMBER 10
#define BLOOD_LAB_NUMBER 10
#define OR_NUMBER 10
#define SURGEON_NUMBER 30
#define NURSE_NUMBER 30
#define SURGEON_LIMIT 5
#define NURSE_LIMIT 5
#define PATIENT_NUMBER 1000


//I used these 3 variables to determine the number of doctors,nurses and or rooms available during the surgical procedure.
int AVAILABLE_OR = OR_NUMBER;
int AVAILABLE_SURGEN = SURGEON_NUMBER;
int AVAILABLE_NURSE = NURSE_NUMBER;

//Mutex and semaphores used
sem_t regist_op;
sem_t gp;
sem_t pharmacy;
sem_t blood_lab;
sem_t restroom;
sem_t cafe;
pthread_mutex_t money_payment;

//Thread
pthread_t tid[PATIENT_NUMBER];
 
//Fixed values found in the document
int HOSPITAL_WALLET = 0;
int WAIT_TIME = 100;
int ARRIVAL_TIME = 100;
int REGISTRATION_TIME = 100;
int GP_TIME = 200;
int PHARMACY_TIME = 100;
int BLOOD_LAB_TIME = 200;
int SURGERY_TIME = 500;
int CAFE_TIME = 100;
int RESTROOM_TIME = 100;
int REGISTRATION_COST = 100;
int PHARMACY_COST = 200; // Calculated randomly between 1 and given value.
int BLOOD_LAB_COST = 200;
int SURGERY_OR_COST = 200;
int SURGERY_SURGEON_COST = 100;
int SURGERY_NURSE_COST = 50;
int CAFE_COST = 200; // Calculated randomly between 1 and given value.
int HUNGER_INCREASE_RATE = 10;
int RESTROOM_INCREASE_RATE = 10;
int Hunger_Meter; // Initialized between 1 and 100 at creation.
int Restroom_Meter; // Initialized between 1 and 100 at creation.


//I used the struct to hold the desired values of the patients.
struct patient {
    int id;
    int hunger;
    int wc;
    int from; // 0->Registration_Operation , 1-> Pharmacy, 2-> Surgery, 3->Blood Lab. 
    int disease; // 1-> Pharmacy, 2-> Surgery, 3->Blood Lab.
};


//The functions used in program
void* registration_Operation(void *item);
int rnd(int randomLimit);
void* GP(struct patient p);
void* Pharmacy(struct patient p);
void* Blood_Lab(struct patient p);
void* Surgery(struct patient p);
void* Restroom(struct patient p);
void* Cafe(struct patient p);

//I kept patients in a struck array
struct patient patients[PATIENT_NUMBER];

int main(){
    
    srand(time(NULL)); //For random numbers
    
    Hunger_Meter = rnd(100); // Initialized between 1 and 100 at creation.
    Restroom_Meter = rnd(100); // Initialized between 1 and 100 at creation.

    // Initializion mutex and semaphore
    sem_init(&regist_op, 0, REGISTRATION_SIZE);
    sem_init(&gp,0,GP_NUMBER);
    sem_init(&pharmacy,0,PHARMACY_NUMBER);
    sem_init(&blood_lab,0,BLOOD_LAB_NUMBER);
    sem_init(&restroom,0,RESTROOM_SIZE);
    sem_init(&cafe,0,CAFE_NUMBER);
    pthread_mutex_init(&money_payment,NULL);
    
    //Creating patients
    for(int i =0;i<PATIENT_NUMBER;i++){
        patients[i].id = i;
        patients[i].hunger = 0;
        patients[i].wc = 0;
        patients[i].from = 0; //0 for Registration Room
        patients[i].disease = rnd(3); // Random determination of the patient's disease
        // 1-> Pharmacy, 2-> Surgery, 3->Blood Lab.
        pthread_create(&(tid[i]), NULL, registration_Operation,(void*)&patients[i]);
        usleep(rnd(ARRIVAL_TIME));
    }
    
    for (int i=0; i<PATIENT_NUMBER; i++){	
		pthread_join(tid[i], NULL);
	}
    //Printing the hospital wallet on the screen after all procedures are completed
    printf("Hospital Wallet: %d \n",HOSPITAL_WALLET);
}

void* registration_Operation(void *item){
    usleep(rnd(WAIT_TIME));
    //Converting struct
    struct patient p = *(struct patient*) item;
    printf("Patient %d: Waiting to enter registration room...\n",p.id);
    
    //Patients waiting during registration
    while(sem_trywait(&regist_op)){
       
        printf("Registation Room is full patient has to wait!\n");
        usleep(rnd(WAIT_TIME));
      
        if(patients[p.id].wc >= Restroom_Meter){
            Restroom(p);
        }
        else if (patients[p.id].hunger >= Hunger_Meter){
            Cafe(p);
        }
        //The patient's increased need for toilet and eating while waiting
        patients[p.id].hunger += rnd(HUNGER_INCREASE_RATE);
        patients[p.id].wc += rnd(RESTROOM_INCREASE_RATE); 
    }
    
    int value;
    sem_getvalue(&regist_op, &value);
    //Suppressing the patient's entry and the number of seats inside
    printf("Patient %d: is now in the registration room, and the registration room can now take care of %d patients.\n", p.id,value);
    
    // Making Registation operation
    usleep(rnd(REGISTRATION_TIME));
    
    //I used of mutex to prevent threads from obstructing each other in payment process.
    pthread_mutex_lock(&money_payment);
    HOSPITAL_WALLET += REGISTRATION_COST;
    printf("Patient %d: paid %d to Registation Room.\n", p.id,REGISTRATION_COST);
    pthread_mutex_unlock(&money_payment);
   
    printf("Patient %d: The patient is now exiting registration room and going to GP.\n", p.id);
    sem_post(&regist_op);
    
    //Patient is going to gp.
    GP(p);
    
    //Closing threads after they are finished.
    pthread_exit(NULL);
    
}

void* Cafe(struct patient p){
    usleep(rnd(WAIT_TIME));
    printf("Patient %d: Waiting to enter Cafe...\n",p.id);
    //I used semaphore for cafe operations of patients.
    sem_wait(&cafe);
    int value;
    sem_getvalue(&cafe, &value);
    printf("Patient %d: is now in the Cafe, and the Cafe can now take care of %d patients.\n", p.id,value);
    
    //Cafe operation
    usleep(rnd(CAFE_TIME));
    
    //Money payment to cafe
    int cost = rnd(CAFE_COST);
    
    //I used of mutex to prevent threads from obstructing each other in payment process.
    pthread_mutex_lock(&money_payment);
    HOSPITAL_WALLET += cost;
    printf("Patient %d: paid %d to Cafe.\n", p.id,cost);
    pthread_mutex_unlock(&money_payment);
    
    //Resetting the patient's hunger
    patients[p.id].hunger = 0;
    sem_post(&cafe);
}

void* Restroom(struct patient p){
    usleep(rnd(WAIT_TIME));
    printf("Patient %d: Waiting to enter Restroom...\n",p.id);
    //I used semaphore for Restroom operations of patients.
    sem_wait(&restroom);
   
    int value;
    sem_getvalue(&restroom, &value);
    printf("Patient %d: is now in the Restroom, and the Restroom can now take care of %d patients.\n", p.id,value);
    
    //Wc operation
    usleep(rnd(RESTROOM_TIME));
    
    //Resetting the patient's wc.
    patients[p.id].wc = 0;
    sem_post(&restroom);
    
}

void* GP(struct patient p){
    usleep(rnd(WAIT_TIME));
    printf("Patient %d: Waiting to enter gp's room...\n",p.id);
    
    //Patients waiting to enter the full gp room.
    while(sem_trywait(&gp)){
       
        printf("Gp's Room is full patient has to wait!\n");
        usleep(rnd(WAIT_TIME));
      
        if(patients[p.id].wc >= Restroom_Meter){
            Restroom(p);
        }
        else if (patients[p.id].hunger >= Hunger_Meter){
            Cafe(p);
        }
        //The patient's increased need for toilet and eating while waiting
        patients[p.id].hunger += rnd(HUNGER_INCREASE_RATE);
        patients[p.id].wc += rnd(RESTROOM_INCREASE_RATE); 
    }
    
    int value;
    sem_getvalue(&gp, &value);
    printf("Patient %d: is now in the gp's room, and the gp's room can now take care of %d patients.\n", p.id,value);
    //Gp operation
    usleep(rnd(GP_TIME));
    sem_post(&gp);
    
    //Examination of the patient by the doctor
    if (p.from == 0) { 
    //from == 0 --> patient come from Registation Room
        //Patient is going to Blood Lab
        if(p.disease == 3){ // 3 --> Blood Lab
            patients[p.id].from = 3;
            p.from = 3;
            printf("Patient %d: The patient is now exiting gp's room and going to Blood Lab.\n", p.id);
            Blood_Lab(p);
        }
        //Patient is going to Pharmacy
        if(p.disease == 1){ // 1 --> Pharmacy 
            patients[p.id].from = 1;
            p.from = 1;
            //Determining whether the patient needs drug therapy
            int medicine = rnd(2);
            if(medicine == 1){
                printf("Patient %d: The patient is now exiting gp's room and going to Pharmacy.\n", p.id);
                Pharmacy(p);
            }
            else{
                printf("Patient %d: was checked by the doctor, the doctor said that there was no need for any drug treatment. The patient is out of the hospital.\n", p.id);
            }
        }
        
        //Patient is going to Surgery
        else{ // 2 --> Surgery 
            patients[p.id].from = 2;
            p.from = 2;
            printf("Patient %d: The patient is now exiting gp's room and going to Surgery.\n", p.id);
            Surgery(p);
        }
    }
    
    else{
        //The doctor's referral of the patient who comes to the control after the surgery or blood draw to the pharmacy.
        
        int medicine = rnd(2); //Determining whether the patient needs drug therapy
        if(medicine == 1){
            printf("Patient %d: The patient is now exiting gp's room and going to Pharmacy.\n", p.id);
            Pharmacy(p);
        }
        else{
            printf("Patient %d: was checked by the doctor, the doctor said that there was no need for any drug treatment. The patient is out of the hospital.\n", p.id);
        }
    }
        
}

void* Pharmacy(struct patient p){
    usleep(rnd(WAIT_TIME));
    printf("Patient %d: Waiting to Pharmacy...\n",p.id);
    
    //Patients waiting to enter the full Pharmacy.
    while(sem_trywait(&pharmacy)){
       
        printf("Pharmacy is full patient has to wait!\n");
        usleep(rnd(WAIT_TIME));
      
        if(patients[p.id].wc >= Restroom_Meter){
            Restroom(p);
        }
        else if (patients[p.id].hunger >= Hunger_Meter){
            Cafe(p);
        }
        //The patient's increased need for toilet and eating while waiting
        patients[p.id].hunger += rnd(HUNGER_INCREASE_RATE);
        patients[p.id].wc += rnd(RESTROOM_INCREASE_RATE); 
    }
   
    int value;
    sem_getvalue(&pharmacy, &value);
    printf("Patient %d: is now in the Pharmacy, and the Pharmacy can now take care of %d patients.\n", p.id,value);
    usleep(rnd(PHARMACY_TIME));
    int cost = rnd(PHARMACY_COST);
    
    //Money payment to Pharmacy
    pthread_mutex_lock(&money_payment);
    HOSPITAL_WALLET += cost;
    printf("Patient %d: paid %d to pharmacy.\n", p.id,cost);
    pthread_mutex_unlock(&money_payment);
    
    
    printf("Patient %d: is out of the hospital.\n", p.id);
    sem_post(&pharmacy);
}

void* Blood_Lab(struct patient p){
    usleep(rnd(WAIT_TIME));
    
    //Patients waiting to enter the full Blood Lab.
    printf("Patient %d: Waiting to Blood Lab...\n",p.id);
    while(sem_trywait(&blood_lab)){
       
        printf("Blood Lab is full patient has to wait!\n");
        usleep(rnd(WAIT_TIME));
      
        if(patients[p.id].wc >= Restroom_Meter){
            Restroom(p);
        }
        else if (patients[p.id].hunger >= Hunger_Meter){
            Cafe(p);
        }
        //The patient's increased need for toilet and eating while waiting
        patients[p.id].hunger += rnd(HUNGER_INCREASE_RATE);
        patients[p.id].wc += rnd(RESTROOM_INCREASE_RATE); 
    }
   
   
    int value;
    sem_getvalue(&blood_lab, &value);
    printf("Patient %d: is now in the Blood Lab, and the Blood Lab can now take care of %d patients.\n", p.id,value);
    
    //Blood operation
    usleep(rnd(BLOOD_LAB_TIME));
    
    //Money payment to blood lab.
    pthread_mutex_lock(&money_payment);
    HOSPITAL_WALLET += BLOOD_LAB_COST;
    printf("Patient %d: paid %d to Blood Lab.\n", p.id,BLOOD_LAB_COST);
    pthread_mutex_unlock(&money_payment);
    
    
    printf("Patient %d: going to gp for check up.\n", p.id);
    sem_post(&blood_lab);
    GP(p);
    
}

void* Surgery(struct patient p){
    usleep(rnd(WAIT_TIME));
    //Random generation of the desired patient and nurse numbers.
    int desired_surgeon_number = rnd(SURGEON_LIMIT);
    int desired_nurse_number = rnd(NURSE_LIMIT);
     
    //Checking the number of avaible doctors, nurses and rooms. If the desired number of doctors, nurses or rooms is not available, I made the patient wait for a  while.
    while(AVAILABLE_SURGEN<desired_surgeon_number || AVAILABLE_NURSE<desired_nurse_number || AVAILABLE_OR<1){
        usleep(WAIT_TIME);
        printf("Patient %d: Waiting for Surgery\n",p.id);
        
        if(patients[p.id].wc >= Restroom_Meter){
            Restroom(p);
        }
        else if (patients[p.id].hunger >= Hunger_Meter){
            Cafe(p);
        }
        //The patient's increased need for toilet and eating while waiting
        patients[p.id].hunger += rnd(HUNGER_INCREASE_RATE);
        patients[p.id].wc += rnd(RESTROOM_INCREASE_RATE);
        
    }
    
    //Reducing the number of available doctors, nurses and rooms to the desired values.
    AVAILABLE_SURGEN = AVAILABLE_SURGEN -desired_surgeon_number;
    AVAILABLE_NURSE = AVAILABLE_NURSE -desired_nurse_number ;
    AVAILABLE_OR--;
    printf("Patient %d: is operating in the operating room with %d doctors and %d nurses. \n",p.id,desired_surgeon_number,desired_nurse_number);
    printf("Patient %d: Currently, there are %d available doctors, %d available nurses and %d available room in the hospital.\n",p.id,AVAILABLE_SURGEN,AVAILABLE_NURSE,AVAILABLE_OR);
    //Surgery operation
    usleep(SURGERY_TIME);
    
    //Money payment to surgery
    int cost = (desired_surgeon_number*SURGERY_SURGEON_COST) +(desired_nurse_number *SURGERY_NURSE_COST) + SURGERY_OR_COST; 
    pthread_mutex_lock(&money_payment);
    HOSPITAL_WALLET += cost;
    printf("Patient %d: paid %d to Surgery.\n", p.id,cost);
    pthread_mutex_unlock(&money_payment);
    
    //Updating the number of doctors, nurses and rooms after the operation.
    AVAILABLE_SURGEN = AVAILABLE_SURGEN +desired_surgeon_number;
    AVAILABLE_NURSE = AVAILABLE_NURSE +desired_nurse_number ;
    AVAILABLE_OR++;
    
    printf("Patient %d: Out of surgery and going to gp for check up.\n", p.id);
    GP(p);
}


//For random number
int rnd(int randomLimit){
    return rand() % randomLimit +1;
}








