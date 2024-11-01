#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <bits/mman-linux.h>
#include <time.h>
#include <signal.h>




sem_t *sem_final;               //signalizuje skibusu, že môže odchádzať z konečnej zastávky
sem_t *sem_boarded;             //signalizuje skibusu, že všetci čakajúci nastúpili a že môže odchádzať
sem_t **sem_stop;               //pole semaforov (jeden na každú zastávku) signalizuje lyžiarom čakajúcim na danej zastávke, že môžu nastupovať 
sem_t *mutex;                   //mutex, ktorý zaručí bezpečné narábanie so zdieľanými premennými (bez neho by premenné nadobúdali chybné hodnoty), používam ho aj pre printf() a fprintf()
sem_t *sem_in_skibus;           //signalizuje lyžiarom v skibuse, že môžu vystupovať

int *boarded        = NULL;     //zdieľaná premenná, ktorá sleduje, koľko lyžiarov je v skibuse
int *anyone_waiting = NULL;     //zdieľaná premenná, ktorá sleduje, koľko lyžiarov čaká na niektorej zo zastávok
int *skibus_cap     = NULL;     //zdieľaná premenná, ktorá sleduje, aktuálnu kapacitu skibusu
int *stops_cap      = NULL;     //zdieľané pole, ktoré sleduje aktuálny počet lyžiarov na každej zastávke
int *num_boarding   = NULL;     //zdieľané premenná, ktoré sleduje koľko lyžiarov by malo nastúpiť (relevantné pre kapacitu skibusu)
int *counter        = NULL;     //zdieľané premenná, ktorá indexuje od 1 poradie procesov na výstupe stdout a proj2.out
FILE *f             = NULL;     //pointer na súbor 'proj2.out'



int init(int stops) {           //inicializácia semaforov a zdieľaných premenných + ošetrenie v prípade chyby

    counter = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    if (counter == MAP_FAILED) {
		fprintf(stderr, "mmap failed.\n");
		return 1;
	}

    skibus_cap = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    if (skibus_cap == MAP_FAILED) {
		fprintf(stderr, "mmap failed.\n");
		return 1;
	}

    num_boarding = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    if (num_boarding == MAP_FAILED) {
		fprintf(stderr, "mmap failed.\n");
		return 1;
	}

    anyone_waiting = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    if (anyone_waiting == MAP_FAILED) {
		fprintf(stderr, "mmap failed.\n");
		return 1;
	}

    boarded = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    if (boarded == MAP_FAILED) {
		fprintf(stderr, "mmap failed.\n");
		return 1;
	}

    stops_cap = mmap(NULL, sizeof(int)  *stops, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    if (stops_cap == MAP_FAILED) {
		fprintf(stderr, "mmap failed.\n");
		return 1;
	}

    sem_stop = mmap(NULL, sizeof(sem_t*)*stops, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    if (sem_stop == MAP_FAILED) {
		fprintf(stderr, "mmap failed.\n");
		return 1;
	}

    sem_boarded = mmap(NULL, sizeof(sem_t), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    if (sem_boarded == MAP_FAILED) {
		fprintf(stderr, "mmap failed.\n");
		return 1;
	}

    sem_final = mmap(NULL, sizeof(sem_t), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    if (sem_final == MAP_FAILED) {
		fprintf(stderr, "mmap failed.\n");
		return 1;
	}

    mutex = mmap(NULL, sizeof(sem_t), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    if (mutex == MAP_FAILED) {
		fprintf(stderr, "mmap failed.\n");
		return 1;
	}

    sem_in_skibus = mmap(NULL, sizeof(sem_t), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    if (sem_in_skibus == MAP_FAILED) {
		fprintf(stderr, "mmap failed.\n");
		return 1;
	}


    for (int i = 0; i < stops; i++) {  

        sem_stop[i] = mmap(NULL, sizeof(sem_t), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
        if (sem_stop[i] == MAP_FAILED) {
            fprintf(stderr, "mmap failed.\n");
            return 1;
        }

    }

    return 0;
}

void cleanup(int stops) {   //uvoľnenie semaforov a zdieľaných premenných

    for (int i = 0; i < stops; i++) {
        sem_destroy(sem_stop[i]);
        munmap(sem_stop[i], sizeof(sem_t));
    }

    sem_destroy(sem_in_skibus);
    sem_destroy(sem_final);
    sem_destroy(mutex);
    sem_destroy(sem_boarded);

    munmap(sem_stop, stops * sizeof(sem_t*));
    munmap(sem_in_skibus, sizeof(sem_t));
    munmap(sem_boarded, sizeof(sem_t));
    munmap(sem_final, sizeof(sem_t));
    munmap(mutex, sizeof(sem_t));
    munmap(anyone_waiting, sizeof(int));
    munmap(boarded, sizeof(int));
    munmap(skibus_cap, sizeof(int));
    munmap(num_boarding, sizeof(int));
    munmap(counter, sizeof(int));
    munmap(stops_cap, sizeof(int)*stops);

}



void go_ski(int idl){

    sem_wait(mutex);

    pid_t curr_process = getpid();

    *counter = *counter + 1;
    fprintf(f, "%d: L %d: going to ski\n", *counter, idl);
    fflush(f);
    printf("%d: L %d: going to ski\n", *counter, idl);
    fflush(stdout);

    *boarded -= 1;
    *anyone_waiting -= 1;
    *skibus_cap += 1;

    if (*boarded == 0) {
        sem_post(sem_final);    //pri každom lyžiarovi sa inkrementuje hodnota sem_final, ktorý zároveň každým vystúpením lyžiara blokuje odchod skibusu, viz. riadok 270
    }
    
    sem_post(mutex);

    kill(curr_process, SIGTERM);

}

void board(int idl, int idz) {

    sem_wait(mutex);

    *counter = *counter + 1;
    fprintf(f, "%d: L %d: boarding\n", *counter, idl);
    fflush(f);
    printf("%d: L %d: boarding\n", *counter, idl);
    fflush(stdout);

    *boarded += 1;
    *num_boarding -= 1;
    *skibus_cap -= 1;
    stops_cap[idz-1] -= 1;


    if (*num_boarding == 0) {
        sem_post(sem_boarded);
        
    }

    sem_post(mutex);


    sem_wait(sem_in_skibus);  //lyžiar čaká po nastúpení do príchodu na konečnú zastávku
    go_ski(idl);              //lyžiar môže ísť lyžovať
 
    
}


void process_skibus(int stops, int time) {
    

    for (int idz = 1; idz <= stops+1; idz++) {
        
        if (idz <= stops) {

            usleep(time);
            
            sem_wait(mutex);

            *counter += 1;
            fprintf(f, "%d: BUS: arrived to %d\n", *counter, idz);
            fflush(f);
            printf("%d: BUS: arrived to %d\n", *counter, idz);
            fflush(stdout);


            if (stops_cap[idz-1] >= *skibus_cap) {   

                *num_boarding = *skibus_cap;       //ak je na zastávke viac lyžiarov, ako sa zmestí do skibusu, počet nastupujúcich je aktuálna kapacita skibusu 

            } else {
                
                *num_boarding = stops_cap[idz-1]; //ak je na zastávke menej lyžiarov, než je aktuálna kap. skibusu, počet nastupujúcich je aktuálny počet čakajúcich na zastávke
                                
            }

            sem_post(mutex);



            if (*num_boarding > 0 && *skibus_cap > 0) {     //semafór, ktorý 'drží' na zastávke lyžiarov sa začne inkrementovat iba ak má kto nastúpiť a zároveň je miesto v skibuse

                while (*num_boarding != 0) {            
                    sem_post(sem_stop[idz-1]);
                    
                }
                sem_wait(sem_boarded);
            }

            sem_wait(mutex);


            *skibus_cap -= *num_boarding;     //od kapacity skibusu sa odčíta počet nastupujúcich 
            stops_cap[idz-1] -= *num_boarding; //od kapacity konkrétnej zastávky sa odčíta počet nastupujúcich

            *counter += 1;
            fprintf(f, "%d: BUS: leaving %d\n", *counter, idz);
            fflush(f);
            printf("%d: BUS: leaving %d\n", *counter, idz);
            fflush(stdout);

            sem_post(mutex);

        }

        else if (idz == stops+1) {

            sem_wait(mutex);

            *counter += 1;
            fprintf(f, "%d: BUS: arrived to final\n", *counter);
            fflush(f);
            printf("%d: BUS: arrived to final\n", *counter);
            fflush(stdout);

            sem_post(mutex);            
    
            if (*boarded > 0) {
                
                while (*boarded != 0) {
                    sem_post(sem_in_skibus);     //v momente príchodu na konečnú zastávku, inkrementujeme semafór, na ktorom čakajú lyžiari, čo nastúpili
                }
                sem_wait(sem_final);        //skibus čaká, kým všetci vystúpia
                
            }

            sem_wait(mutex);

            *counter += 1;
            fprintf(f, "%d: BUS: leaving final\n", *counter);
            fflush(f);
            printf("%d: BUS: leaving final\n", *counter);
            fflush(stdout);

            sem_post(mutex);

        }
    }
}


void process_skier(int time, int idl, int idz){

    sem_wait(mutex);

    *counter += 1;
    fprintf(f, "%d: L %d: started\n", *counter, idl);
    fflush(f);
    printf("%d: L %d: started\n", *counter, idl);
    fflush(stdout);

    *anyone_waiting += 1;

    usleep(time);

    *counter += 1;
    fprintf(f, "%d: L %d: arrived to %d\n", *counter, idl, idz);
    fflush(f);
    printf("%d: L %d: arrived to %d\n", *counter, idl, idz);
    fflush(stdout);

    stops_cap[idz-1] += 1;

    sem_post(mutex);



    sem_wait(sem_stop[idz-1]); //lyžiar po príchode čaká na semafóre na príslušnej zastávke, kým príde skibus
    board(idl, idz);           //lyžiar môže nastúpiť        

}
 


int main(int argc, char *argv[]) {
	
    if (argc != 6){
        fprintf(stderr, "invalid number of arguments.\n");
        return 1;
    }

    int num_of_skiers    = atoi(argv[1]);      //počet lyžiarov
    int num_of_stops     = atoi(argv[2]);      //počet zastávok
    int skier_usleep     = atoi(argv[4]);      //cesta lyžiara po raňajkách na zastávku
    int skibus_usleep    = atoi(argv[5]);      //čas cesty skibusu medzi zastávkami


;

    if (init(num_of_stops) == 1){           //ak niečo pri inicializácií semaforov a zdieľaných premenných zlyhá, program skončí chybovým hlásením 
        fprintf(stderr, "init fail.\n");
        cleanup(num_of_stops);
        return 1;
    }

    int sc = atoi(argv[3]);                             //kapacita skibusu
    skibus_cap = &sc;

    if (*skibus_cap < 10 || *skibus_cap > 100) {        //kontrola povolenej kapacity skibusu
        fprintf(stderr, "invalid skibus capacity.\n");
        cleanup(num_of_stops);
        return 1;
    }

    if (num_of_skiers > 20000) {                        //kontrola povoleného počtu lyžiarov
        fprintf(stderr, "invalid number of skiers.\n");
        cleanup(num_of_stops);
        return 1;
    }

    if (num_of_stops < 1 || num_of_stops > 10) {        //kontrola povoleného počtu zastávok
        fprintf(stderr, "invalid number of stops.\n");
        cleanup(num_of_stops);
        return 1;
    }

    if (skier_usleep < 0 || skier_usleep > 10000) {      //kontrola povoleného času cesty lyžiara na zastávku
        fprintf(stderr, "invalid skier travel time.\n");
        cleanup(num_of_stops);
        return 1;
    }

    if (skibus_usleep < 0 || skibus_usleep > 1000) {      //kontrola povoleného času cesty skibusu medzi zástavkami
        fprintf(stderr, "invalid skibus travel time.\n");
        cleanup(num_of_stops);
        return 1;
    }


    for (int s = 0; s < num_of_stops; s++) {  //for cyklus inicializuje semafór na každej zastávke    
        sem_init(sem_stop[s],1,0);        
    }


    /* všetky semafóry inicializujem na hodnotu 0, okrem mutexu, 
    ktorý, aby púšťal procesy vždy po jednom, musí mať 
    na začiatku hodnotu 1 */
    sem_init(sem_final, 1,0);   
    sem_init(sem_in_skibus,1,0);
    sem_init(sem_boarded,1,0);
    sem_init(mutex, 1,1);   

    f = fopen("proj2.out", "w");

    /* funkcia srand() generuje náhodný seed pre 
    rand() funkcie podľa systémového času, zaisťujem 
    tým náhodnosť usleep() časov, ale nie je úplne nutná */
    srand(time(NULL));


    pid_t skibus = fork();
    
    if (skibus < 0) {
        fprintf(stderr, "fork fail.\n");
        return 1;

    } else if (skibus == 0) {  

        pid_t bus = getpid();

        int travel_time = rand() % skibus_usleep + 1;;
        
        sem_wait(mutex);

        *counter = *counter + 1;
        fprintf(f,"%d: BUS: started\n", *counter);
        fflush(f);
        printf("%d: BUS: started\n", *counter);
        fflush(stdout);

        sem_post(mutex);

        do {
            process_skibus(num_of_stops, travel_time);
        } while (*anyone_waiting > 0);                  //do {}while () cyklus zaručí, že ak ešte niekto čaká na niektorej zo zastávok, skibus urobí nové kolo


        sem_wait(mutex);

        *counter = *counter + 1;
        fprintf(f, "%d: BUS: finish\n", *counter);
        fflush(f);
        printf("%d: BUS: finish\n", *counter);
        fflush(stdout);

        kill(bus, SIGTERM);

        sem_post(mutex);
        
    
    } else {    //parent proces
    

        for (int s = 1; s <= num_of_skiers; s++) {
        
            int wait_time = rand() % skier_usleep + 1;   
            int stop_id   = rand() % num_of_stops + 1;  

            pid_t skier = fork();


            if (skier < 0) {
                fprintf(stderr, "fork fail.\n");
                return 1;

            } else if (skier == 0) {
              
                process_skier(wait_time, s, stop_id);

            }

        }



    }

    while (wait(NULL) > 0) {};

    cleanup(num_of_stops);
    fclose(f);
	return 0;
}

