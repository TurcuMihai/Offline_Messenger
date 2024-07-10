
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#define PORT 2023
#define IP "127.0.0.1"



char buffer[300] ="";
char comanda [100];
int size;
char id_raspuns[20];
char text[200];
char cuvant[30];
char raspuns[1000];
char nume_utilizator[30];
char nume_prieten[30];
char parola[30];
int logat = 0;
int job_done = 0;
int sd;


void *receptor(void * arg);

pthread_t listener;
pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER;
 
int main ()
{
  struct sockaddr_in server; 

  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("Eroare socket().\n");
      return errno;
    }


  server.sin_family = AF_INET;
  server.sin_addr.s_addr = inet_addr(IP);
  server.sin_port = htons(PORT);
  
    if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
    {
      printf("[client]Eroare the connect().\n");
      return errno;
    }

    printf("Asteptam sa va conectati.\n");
    while(1)
    {
        if(job_done == 0)
        {
            printf("Va rugam sa introduceti comanda dumneavoasta: ");
            fgets(comanda,100,stdin);
            comanda[strlen(comanda) - 1] = '\0';
            size = strlen(comanda);
            if(write(sd,&comanda,sizeof(int)) == -1)
            {
                printf("Eroare la write.\n");
                exit(1);
            }
            if(write(sd,comanda,size) == -1)
            {
                printf("Eroare la write.\n");
                exit(1);
            }
            if(strcmp(comanda,"Conectare") == 0)
            {
                if(logat == 0)
                {
                    do
                    {
                        printf("Introduceti numele de utilizator: ");
                        fgets(nume_utilizator,30,stdin);
                        nume_utilizator[strlen(nume_utilizator) - 1] = '\0';
                        size = strlen(nume_utilizator);
                        if(write(sd,&size,sizeof(int)) == -1)
                        {
                            printf("Eroare la write.\n");
                            exit(1);
                        }
                        if(write(sd,nume_utilizator,size) == -1)
                        {
                            printf("Eroare la write.\n");
                            exit(1);
                        }
                        if(read(sd,&size,sizeof(int)) == -1)
                        {
                            printf("Eroare la read.\n");
                            exit(1);
                        }
                        if(read(sd,raspuns,size) == -1)
                        {
                            printf("Eroare la read.\n");
                            exit(1);
                        }
                        raspuns[size] = '\0';
                        if(strcmp(raspuns,"Nume de utilizator incorect.") == 0)
                        {
                            printf("Nume de utilizator incorect.\n");
                        }
                    }while(strcmp(raspuns,"Nume de utilizator incorect.") == 0);
                    
                    printf("Nume de utilizator corect.\n");
                    
                    do
                    {
                        printf("Introduceti parola: ");
                        fgets(parola,30,stdin);
                        parola[strlen(parola) - 1] = '\0';
                        size = strlen(parola);
                        if(write(sd,&size,sizeof(int)) == -1)
                        {
                            printf("Eroare la write.\n");
                            exit(1);
                        }
                        if(write(sd,parola,size) == -1)
                        {
                            printf("Eroare la write.\n");
                            exit(1);
                        }
                        if(read(sd,&size,sizeof(int)) == -1)
                        {
                            printf("Eroare la read.\n");
                            exit(1);
                        }
                        if(read(sd,raspuns,size) == -1)
                        {
                            printf("Eroare la read.\n");
                            exit(1);
                        }
                    
                        raspuns[size] = '\0'; 

                        if(strcmp(raspuns,"Parola incorecta.") == 0)
                        {
                            printf("Parola incorecta.\n");
                        }
                    
                    }while(strcmp(raspuns,"Parola incorecta.") == 0);
                    
                    printf("Parola corecta!\n");

                    if(read(sd,&size,sizeof(int)) == -1)
                    {
                        printf("Eroare la read.\n");
                        exit(1);
                    }
                    if(read(sd,raspuns,size) == -1)
                    {
                        printf("Eroare la read.\n");
                        exit(1);
                    }
                    raspuns[size] = '\0';

                    printf("%s\n",raspuns);


                    logat = 1;
                    pthread_create(&listener,NULL,&receptor,(void *)sd);
                    pthread_detach(listener);
                }
                else
                {
                    printf("Esti deja conectat.\n");
                }
            } 
            else
            if(strcmp(comanda,"Exit") == 0)
            {
                logat = 0;
                exit(1);
            }
            else
            if(strcmp(comanda,"Deconectare") == 0)
            {
                if(logat == 0)
                {
                    printf("Nu esti conectat.\n");
                }
                else
                {
                    logat = 0;
                    printf("Te-ai deconectat cu succes!\n");
                }
            }
            else
            if(strcmp(comanda,"Vizualizare conversatie") == 0)
            {
                if ( logat == 1)
                {
                    job_done = 1;
                    printf("Introduceti numele utilizatorului: ");
                    fgets(nume_prieten,30,stdin);
                    nume_prieten[strlen(nume_prieten) - 1] = '\0';
                    printf("%s\n",nume_prieten);
                    size = strlen(nume_prieten);
                    if(write(sd,&size,sizeof(int)) == -1)
                    {
                        printf("Eroare la write.\n");
                        exit(1);
                    }
                    if(write(sd,nume_prieten,size) == -1)
                    {
                        printf("Eroare la write.\n");
                        exit(1);
                    }

                }
                else
                {
                    printf("Nu sunteti conectat.\n");
                }
            }
            else
            if(strcmp(comanda,"Trimite mesaj") == 0)
            {
                if(logat == 1)
                {
                    job_done = 1;
                    printf("Introduceti numele utilizatorului: ");
                    fgets(nume_prieten,30,stdin);
                    nume_prieten[strlen(nume_prieten) - 1] = '\0';
                    size = strlen(nume_prieten);
                    if(write(sd,&size,sizeof(int)) == -1)
                    {
                        printf("Eroare la write.\n");
                        exit(1);
                    }
                    if(write(sd,nume_prieten,size) == -1)
                    {
                        printf("Eroare la write.\n");
                        exit(1);
                    }


                }
                else
                {
                    printf("Nu esti conectat!\n");
                }
            }
            else
            if(strcmp(comanda,"Trimite raspuns") == 0)
            {
                if(logat == 1)
                {
                    job_done = 1;
                    printf("Introduceti ID-ul mesajului la care doriti sa raspundeti: ");
                    fgets(id_raspuns,30,stdin);
                    id_raspuns[strlen(id_raspuns) - 1] = '\0';
                    size = strlen(id_raspuns);
                    if(write(sd,&size,sizeof(int)) == -1)
                    {
                        printf("Eroare la write.\n");
                        exit(1);
                    }
                    if(write(sd,id_raspuns,size) == -1)
                    {
                        printf("Eroare la write.\n");
                        exit(1);
                    }

                }
                else
                {
                    printf("Nu esti conectat!\n");
                }
            }
            else
            {
                printf("Comanda invalida!\n");
            }
        }
    }
    close(sd);
}

void *receptor(void *arg)
{
    char check[10];
    int sd = (int) arg;

        while(logat == 1)
        {
            if(read(sd,&size,sizeof(int)) == -1)
            {
                printf("Eroare la read.(1)\n");
                exit(1);
            }
            if(read(sd,raspuns,size) == -1)
            {
                printf("Eroare la read.(2)\n");
                exit(1);
            }
            raspuns[size] = '\0';

            strncpy(check,raspuns,7);
            check[7] = '\0';

            if(strcmp(raspuns,"Nume de utilizator valid.") == 0)
            { 
                printf("Nume de utilizator valid.\n");
                printf("Introduceti mesajul: ");
                fgets(text,200, stdin);
                text[strlen(text) - 1] = '\0';
                            
                size = strlen(text);
                if(write(sd,&size,sizeof(int)) == -1)
                {
                    printf("Eroare la write.\n");
                    exit(1);
                }
                if(write(sd,text,size)  == -1)
                {
                    printf("Eroare la write.\n");
                    exit(1);
                }
                job_done = 0;
            }
            else
            if(strcmp(raspuns,"Nume de utilizator invalid.") == 0)
            {
                printf("Nume de utilizator invalid.\n");
                job_done = 0;
            }
            else
            if(strcmp(raspuns,"Id valid.") == 0)
            {
                printf("Introduceti mesajul: ");
                fgets(text,200, stdin);
                text[strlen(text) - 1] = '\0';
                size = strlen(text);
                if(write(sd,&size,sizeof(int)) == -1)
                {
                    printf("Eroare la write.\n");
                    exit(1);
                }
                if(write(sd,text,size)  == -1)
                {
                    printf("Eroare la write.\n");
                    exit(1);
                }        
                job_done = 0;
            }
            else
            if(strcmp(raspuns,"Id invalid.") == 0)
            {
                printf("Id invalid.\n");
                job_done = 0;
            }
            else
            if(strcmp(raspuns,"Acest utilizator nu exista.") == 0)
            {
                printf("%s\n",raspuns);
                job_done = 0;
            }
            else
            if(strcmp(check,"mesaje:") == 0)
            {
                strcpy(raspuns,raspuns+7);
                printf("%s\n",raspuns);
                job_done = 0;
            }
            else
            if(strcmp(raspuns,"O zi buna.") == 0)
            {

            }
            else
            {
                printf("%s\n",raspuns);
            }
        }
        pthread_detach(listener);

}
