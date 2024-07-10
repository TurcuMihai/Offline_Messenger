#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <sqlite3.h>

#define PORT 2023
#define NR_TH 5

static void *treat(void *); 

typedef struct {
	pthread_t idThread; //id-ul thread-ului
	int thCount; //nr de conexiuni servite
}Thread;

Thread *threadsPool; //un array de structuri Thread

sqlite3 *db;
int sd; //descriptorul de socket de ascultare
int nthreads;//numarul de threaduri
int size;
char *raspuns;
int vf;
char *err_msg;

pthread_mutex_t mlock=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t data_base =PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lacate[NR_TH]=PTHREAD_MUTEX_INITIALIZER;

int vector_fd[NR_TH];
char vector_utilizatori[NR_TH][40];

void raspunde(int cl,int idThread);
int verificare_nume_utilizator(char * nume_utilizator);
int verificare_parola(char * nume_utilizator, char *parola);
void offline_messages(int cl, char * nume_utilizator);
void set_online(char * nume_utilizator);
void set_offline(char * nume_utilizator);
void afisare_conversatie(int cl, char *nume_utilizator, char *nume_prieten);
void inserare_offline(char *nume_utilizator, char *nume_prieten , char *buffer);
int check_status(char *nume_utilizator);
int get_number(char * table);
void inserare_mesaj(char* nume_utilizator, char* nume_prieten,char* buffer);
int verificare_id(char * nume_utilizator, char * id);
void inserare_raspuns(char *id, char * nume_utilizator, char *nume_prieten, char *raspuns);
void stergere_mesaje(char * nume_utilizator);

int main (int argc, char *argv[])
{
    for(int i = 0; i < NR_TH; i++)
    {
        vector_fd[i] = -1;
        strcpy(vector_utilizatori[i],"");
    }

    vf = sqlite3_open("baza.db",&db);
	if(vf != SQLITE_OK)
	{
		printf("[server]Eroare la deschiderea bazei de date!\n");
		exit(1);
	}

    struct sockaddr_in server;	
    void threadCreate(int);

    threadsPool = calloc(sizeof(Thread),nthreads);


  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("[server]Eroare la socket().\n");
      return errno;
    }
  int on=1;
  setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
  
  bzero (&server, sizeof (server));


    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl (INADDR_ANY);
    server.sin_port = htons (PORT);
  
  if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
    {
      perror ("[server]Eroare la bind().\n");
      return errno;
    }

  if (listen (sd, 2) == -1)
    {
      perror ("[server]Eroare la listen().\n");
      return errno;
    }

   int i;
   for(i=0; i<NR_TH;i++) threadCreate(i);

  for ( ; ; ) 
  {
	printf ("[server]Asteptam la portul %d...\n",PORT);
        pause();				
  }
}

void threadCreate(int i)
{
	void *treat(void *);
	
	pthread_create(&threadsPool[i].idThread,NULL,&treat,(void*)i);
	return; /* threadul principal returneaza */
}

void *treat(void * arg)
{		
		int client;
		        
		struct sockaddr_in from; 
 	    bzero (&from, sizeof (from));
 		printf ("[thread]- %d - pornit...\n", (int) arg);fflush(stdout);

		for( ; ; )
		{
			int length = sizeof (from);
			pthread_mutex_lock(&mlock);
			//printf("Thread %d trezit\n",(int)arg);
			if ( (client = accept (sd, (struct sockaddr *) &from, &length)) < 0)
				{
	 			 perror ("[thread]Eroare la accept().\n");	  			
				}
			pthread_mutex_unlock(&mlock);
			threadsPool[(int)arg].thCount++;

			raspunde(client,(int)arg); //procesarea cererii
			/* am terminat cu acest client, inchidem conexiunea */
			close (client);			
		}	
}
    
void raspunde(int cl,int idThread)
{
    int vf;
    char buffer[500];
    char nume_utilizator[30];
    char nume_prieten[30];
    char parola[30];
    int logat = 0;

    while (1)
    {
        if(read(cl,&size,sizeof(int)) == -1)
        {
            printf("Eroare la read.\n");
            exit(1);
        }
        raspuns =(char *)malloc(size);
        if(read(cl,raspuns,size) == -1)
        {
            printf("Eroare la read.\n");
            exit(1);
        }
        if(strcmp(raspuns,"Conectare") == 0 && logat == 0)
        {   
            do
            {
                if(read(cl,&size,sizeof(int)) == -1)
                {
                    printf("Eroare la read.\n");
                    exit(1);
                }
                if(read(cl,nume_utilizator,size) == -1)
                {
                    printf("Eroare la read.\n");
                    exit(1);
                }
                nume_utilizator[size] = '\0';
                // verificam nume de utilizator
                if (verificare_nume_utilizator(nume_utilizator) == 1)
                {
                    size = strlen("Nume de utilizator corect.");
                    if(write(cl,&size,sizeof(int)) == -1)
                    {
                        printf("Eroare la write.\n");
                        exit(1);
                    }
                    if(write(cl,"Nume de utilizator corect.",size) == -1)
                    {
                        printf("Eroare la write.\n");
                        exit(1);
                    }
                }
                else
                {
                    size = strlen("Nume de utilizator incorect.");
                    if(write(cl,&size,sizeof(int)) == -1)
                    {
                        printf("Eroare la write.\n");
                        exit(1);
                    }
                    if(write(cl,"Nume de utilizator incorect.",size) == -1)
                    {
                        printf("Eroare la write.\n");
                        exit(1);
                    }
                }
            }while(verificare_nume_utilizator(nume_utilizator) == 0);

            do
            {
                if(read(cl,&size,sizeof(int)) == -1)
                {
                    printf("Eroare la read.\n");
                    exit(1);
                }
                if(read(cl,parola,size) == -1)
                {
                    printf("Eroare la read.\n");
                    exit(1);
                }
                parola[size] = '\0';
                if (verificare_parola(nume_utilizator,parola) == 1)
                {
                    size = strlen("Parola corecta.");
                    if(write(cl,&size,sizeof(int)) == -1)
                    {
                        printf("Eroare la write.\n");
                        exit(1);
                    }
                    if(write(cl,"Parola corecta.",size) == -1)
                    {
                        printf("Eroare la write.\n");
                        exit(1);
                    }
                }
                else
                {
                    size = strlen("Parola incorecta.");
                    if(write(cl,&size,sizeof(int)) == -1)
                    {
                        printf("Eroare la write.\n");
                        exit(1);
                    }
                    if(write(cl,"Parola incorecta.",size) == -1)
                    {
                        printf("Eroare la write.\n");
                        exit(1);
                    }
                }
            }while(verificare_parola(nume_utilizator,parola) == 0);
            
            // APELAM FUNCTIA PENTRU A AFISA MESAJELE OFFLINE

            logat = 1;
            vector_fd[idThread] = cl;
            strcpy(vector_utilizatori[idThread],nume_utilizator);
            printf("%d\n",vector_fd[idThread]);
            printf("%s\n",vector_utilizatori[idThread]);
            offline_messages(cl,nume_utilizator);
            set_online(nume_utilizator);
            stergere_mesaje(nume_utilizator);


        }
        else
        if(strcmp(raspuns,"Exit") == 0)
        {

            if(logat == 1)
                set_offline(nume_utilizator);
            logat = 0;
            strcpy(vector_utilizatori[idThread],"");
            vector_fd[idThread] = -1;
            printf("Utilizatorul %d deconectat.\n",idThread);
            return;
        }
        else
        if(strcmp(raspuns,"Deconectare") == 0)
        {
            logat = 0;
            size = strlen("O zi buna.");
            if(write(cl,&size,sizeof(int)) == -1)
            {
                printf("Eroare la scriere.");
                exit(1);
            }
            if(write(cl,"O zi buna.",size) == -1)
            {
                printf("Eroare la write.");
                exit(1);
            }
            set_offline(nume_utilizator);
        }
        else
        if(strcmp(raspuns,"Vizualizare conversatie") == 0 && logat == 1)
        {
                if(read(cl,&size,sizeof(int)) == -1)
                {
                    printf("Eroare la read.\n");
                    exit(1);
                }
                if(read(cl,nume_prieten,size) == -1)
                {
                    printf("Eroare la read.\n");
                    exit(1);
                }
                nume_prieten[size] =  '\0';
                if(verificare_nume_utilizator(nume_prieten) == 0)
                {
                    size = strlen("Acest utilizator nu exista.");
                    if(write(cl,&size,sizeof(int)) == -1)
                    {
                        printf("Eroare la write.\n");
                        exit(1);
                    }
                    if(write(cl,"Acest utilizator nu exista.",size) == -1)
                    {
                        printf("Eroare la write.\n");
                        exit(1);
                    }
                }
                else
                {
                    printf("AM INTRAT AICI!\n");
                    afisare_conversatie(cl,nume_utilizator,nume_prieten);
                
                }
        }
        else
        if(strcmp(raspuns,"Trimite mesaj") == 0 && logat == 1)
        {
                if(read(cl,&size,sizeof(int)) == -1)
                {
                    printf("Eroare la read.\n");
                    exit(1);
                }
                if(read(cl,nume_prieten,size) == -1)
                {
                    printf("Eroare la read.\n");
                    exit(1);
                }
                nume_prieten[size] = '\0';
                if(verificare_nume_utilizator(nume_prieten) == 1)
                {
                    size = strlen("Nume de utilizator valid.");
                    if(write(cl,&size,sizeof(int)) == -1)
                    {
                        printf("Eroare la write.\n");
                        exit(1);
                    }
                    if(write(cl,"Nume de utilizator valid.",size) == -1)
                    {
                        printf("Eroare la write.\n");
                        exit(1);
                    }
                    if(read(cl,&size,sizeof(int)) == -1)
                    {
                        printf("Eroare la read.\n");
                        exit(1);
                    }
                    if(read(cl,buffer,size) == -1)
                    {
                        printf("Eroare la read.\n");
                        exit(1);
                    }
                    buffer[size] = '\0';
                    printf("%s\n",buffer);
                    pthread_mutex_lock(&data_base);
                    if (check_status(nume_prieten) == 1)
                    {
                        int i;
                        int id = get_number("messages");
                        id++;
                        char id_char[10];
                        sprintf(id_char,"%d",id);
                        char trimit[100]= "\n";
                        strcat(trimit,nume_utilizator);
                        strcat(trimit," ti-a trimis mesajul cu ID-ul ");
                        strcat(trimit,id_char);
                        strcat(trimit," :\n");
                        strcat(trimit,buffer);
                        for(i = 0; i < NR_TH; i++)
                        {
                            if(strcmp(nume_prieten,vector_utilizatori[i]) == 0)
                            {
                                break;
                            }
                        }
                        size = strlen(trimit);
                        if(write(vector_fd[i],&size,sizeof(int)) == -1)
                        {
                            printf("Eroare la write. in fd\n");
                            exit(1);
                        }
                        if(write(vector_fd[i],trimit,size) == -1)
                        {
                            printf("Eroare la write. in fd\n");
                            exit(1);
                        }
                    }
                    inserare_mesaj(nume_utilizator,nume_prieten,buffer);
                    pthread_mutex_unlock(&data_base);
                }
                else
                {
                    size = strlen("Nume de utilizator invalid.");
                    if(write(cl,&size,sizeof(int)) == -1)
                    {
                        printf("Eroare la write.\n");
                        exit(1);
                    }
                    if(write(cl,"Nume de utilizator invalid.",size) == 0)
                    {
                        printf("Eroare la write.\n");
                        exit(1);
                    }
                }
        }
        else
        if(strcmp(raspuns,"Trimite raspuns") == 0 && logat == 1)
        {
            char id[10];
            if(read(cl,&size,sizeof(int)) == -1)
            {
                printf("Eroare la read.\n");
                exit(1);
            }
            if(read(cl,raspuns,size) == -1)
            {
                printf("Eroare la read.\n");
                exit(1);
            }
            raspuns[size] = '\0';
            strcpy(id,raspuns);
            if(verificare_id(nume_utilizator, raspuns) == 1)
            {
                size = strlen("Id valid.");
                if(write(cl,&size,sizeof(int)) == -1)
                {
                    printf("Eroare a write.\n");
                    exit(1);
                }
                if(write(cl,"Id valid.",size) == -1)
                {
                    printf("Eroare la write.\n");
                    exit(1);
                }
                if(read(cl,&size,sizeof(int)) == -1)
                {
                    printf("Eroare la read.\n");
                    exit(1);
                }
                if(read(cl,raspuns,size) == -1)
                {
                    printf("Eroare la read.\n");
                    exit(1);
                }
                raspuns[size] = '\0';

                sqlite3_stmt *res;
                int vf;
                char sql[100]="SELECT sender from messages where id = '";
                strcat(sql,id);
                strcat(sql,"'");
                vf = sqlite3_prepare_v2(db,sql,-1,&res,0);
                if(vf != SQLITE_OK)
                {
                    printf("Eroare la interogare.(3)\n");
                    sqlite3_close(db);
                    exit(1);
                }
                vf = sqlite3_step(res);
                strcpy(nume_prieten,sqlite3_column_text(res,0));

                pthread_mutex_lock(&data_base);

                if(check_status(nume_prieten) == 1)
                {
                        int i;
                        int id_mesaj = get_number("messages");
                        id_mesaj++;
                        char id_char[10];
                        sprintf(id_char,"%d",id_mesaj);
                        char trimit[100]= "\n";
                        strcat(trimit,nume_utilizator);
                        strcat(trimit," ti-a trimis raspunsul cu ID-ul ");
                        strcat(trimit,id_char);
                        strcat(trimit," la mesajul cu ID-ul ");
                        strcat(trimit,id);
                        strcat(trimit," :\n");
                        strcat(trimit,raspuns);
                        strcat(trimit,"\n");
                        for(i = 0; i < NR_TH; i++)
                        {
                            if(strcmp(nume_prieten,vector_utilizatori[i]) == 0)
                            {
                                break;
                            }
                        }
                        size = strlen(trimit);
                        if(write(vector_fd[i],&size,sizeof(int)) == -1)
                        {
                            printf("Eroare la write. in fd\n");
                            exit(1);
                        }
                        if(write(vector_fd[i],trimit,size) == -1)
                        {
                            printf("Eroare la write. in fd\n");
                            exit(1);
                        }
                }
                pthread_mutex_unlock(&data_base);
                inserare_raspuns(id,nume_utilizator,nume_prieten,raspuns);
            }
            else
            {
                size = strlen("Id invalid.");
                if(write(cl,&size,sizeof(int)) == -1)
                {
                    printf("Eroare a write.\n");
                    exit(1);
                }
                if(write(cl,"Id invalid.",size) == -1)
                {
                    printf("Eroare la write.\n");
                    exit(1);
                }
            }
        }

    }
}

void stergere_mesaje(char * nume_utilizator)
{
    sqlite3_stmt *res;
    int vf;
    char sql[100] = "DELETE from offline_messages where receiver = '";
    strcat(sql,nume_utilizator);
    strcat(sql,"'");
    vf = sqlite3_exec(db,sql,0,0,&err_msg);
    if(vf != SQLITE_OK)
    {
        printf("Eroare la inserare in bd!\n");
        exit(1);
    }

}

void inserare_raspuns(char *id, char * nume_utilizator, char * nume_prieten, char *buffer)
{

    int id2 = atoi(id);

    int id1;
    id1 = get_number("messages");
    id1++;
    int vf;
    char *query = sqlite3_mprintf("INSERT INTO messages (id,sender,receiver,text,answer_for) VALUES ('%d','%s','%s','%s','%d');",id1,nume_utilizator,nume_prieten,buffer,id2);
    vf = sqlite3_exec(db,query,0,0,&err_msg);
    if(vf != SQLITE_OK)
    {
        printf("Eroare la inserare in bd!\n");
        exit(1);
    }
    if(check_status(nume_prieten) == 0)
    {
        int vf;
        char *query = sqlite3_mprintf("INSERT INTO offline_messages (id,sender,receiver,text,answer_for) VALUES ('%d','%s','%s','%s','%d');",id1,nume_utilizator,nume_prieten,buffer,id2);
        vf = sqlite3_exec(db,query,0,0,&err_msg);
        if(vf != SQLITE_OK)
        {
            printf("Eroare la inserare in bd!\n");
            exit(1);
        }
    }

}

int verificare_id(char *nume_utilizator, char *id)
{
    sqlite3_stmt *res;
    int vf;
    char sql[100]="SELECT id , receiver from messages where id = '";
    strcat(sql,id);
    strcat(sql,"'");
    vf = sqlite3_prepare_v2(db,sql,-1,&res,0);
    if(vf != SQLITE_OK)
    {
        printf("Eroare la interogare.(3)\n");
        sqlite3_close(db);
        exit(1);
    }
    vf = sqlite3_step(res);
    int id1;
    id1 = atoi(id);
    while(vf != SQLITE_DONE)
    {
        if(sqlite3_column_int(res,0) == id1 && strcmp(sqlite3_column_text(res,1),nume_utilizator) == 0)
        {
            return 1;
        }
        vf = sqlite3_step(res);
    }
    return 0; 
}

int check_status(char * nume_utilizator)
{
    sqlite3_stmt *res;
    int vf;
    char sql[100]="SELECT status from utilizatori where name = '";
    strcat(sql,nume_utilizator);
    strcat(sql,"'");
    vf = sqlite3_prepare_v2(db,sql,-1,&res,0);
    if(vf != SQLITE_OK)
    {
        printf("Eroare la interogare.(1)\n");
        sqlite3_close(db);
        exit(1);
    }
    vf = sqlite3_step(res);
    while(vf != SQLITE_DONE)
    {
        if(strcmp("online",sqlite3_column_text(res,0)) == 0)
        {
            return 1;
        }
        vf = sqlite3_step(res);
    }
    return 0;
}

int get_number(char * table)
{
    sqlite3_stmt *res;
    int vf;
    char sql[100]="SELECT count(*) from ";
    strcat(sql,table);
    strcat(sql," ;");
    vf = sqlite3_prepare_v2(db,sql,-1,&res,0);
    if(vf != SQLITE_OK)
    {
        printf("Eroare la interogare.(2)\n");
        sqlite3_close(db);
        exit(1);
    }
    vf = sqlite3_step(res);
    
    return sqlite3_column_int(res,0);

}

void inserare_mesaj(char* nume_utilizator, char* nume_prieten,char* buffer)
{  
    int id;
    id = get_number("messages");
    id++;
    int vf;
    char *query = sqlite3_mprintf("INSERT INTO messages (id,sender,receiver,text,answer_for) VALUES ('%d','%s','%s','%s','%d');",id,nume_utilizator,nume_prieten,buffer,0);
    vf = sqlite3_exec(db,query,0,0,&err_msg);
    if(vf != SQLITE_OK)
    {
        printf("Eroare la inserare in bd!\n");
        exit(1);
    }
    if(check_status(nume_prieten) == 0)
    {
        int vf;
        char *query = sqlite3_mprintf("INSERT INTO offline_messages (id,sender,receiver,text,answer_for) VALUES ('%d','%s','%s','%s','%d');",id,nume_utilizator,nume_prieten,buffer,0);
        vf = sqlite3_exec(db,query,0,0,&err_msg);
        if(vf != SQLITE_OK)
        {
            printf("Eroare la inserare in bd!\n");
            exit(1);
        }
    }
}

int verificare_nume_utilizator(char * nume_utilizator)
{
    sqlite3_stmt *res;
    int vf;
    char sql[100]="SELECT name from utilizatori where name = '";
    strcat(sql,nume_utilizator);
    strcat(sql,"'");
    vf = sqlite3_prepare_v2(db,sql,-1,&res,0);
    if(vf != SQLITE_OK)
    {
        printf("Eroare la interogare.(3)\n");
        sqlite3_close(db);
        exit(1);
    }
    vf = sqlite3_step(res);
    while(vf != SQLITE_DONE)
    {
        if(strcmp(nume_utilizator,sqlite3_column_text(res,0)) == 0)
        {
            return 1;
        }
        vf = sqlite3_step(res);
    }
    return 0;
}

int verificare_parola(char * nume_utilizator, char *parola)
{
    sqlite3_stmt *res;
    int vf;
    char sql[100]="SELECT password from utilizatori where name = '";
    strcat(sql,nume_utilizator);
    strcat(sql,"'");
    vf = sqlite3_prepare_v2(db,sql,-1,&res,0);
    if(vf != SQLITE_OK)
    {
        printf("Eroare la interogare.\n");
        sqlite3_close(db);
        exit(1);
    }
    vf = sqlite3_step(res);
    while(vf != SQLITE_DONE)
    {
        if(strcmp(parola,sqlite3_column_text(res,0)) == 0)
        {
            return 1;
        }
        vf = sqlite3_step(res);
    }
    return 0;
}

void offline_messages(int cl, char *nume_utilizator)
{
    sqlite3_stmt *res;
    int vf;
    char buffer[400]="\nHei! Cat timp ai fost offline ai primit urmatoarele mesaje!\n\n";
    char sql[100]="SELECT sender, text, answer_for from offline_messages where receiver = '";
    strcat(sql,nume_utilizator);
    strcat(sql,"'");
    vf = sqlite3_prepare_v2(db,sql,-1,&res,0);
    if(vf != SQLITE_OK)
    {
        printf("Eroare la interogare.(4)\n");
        sqlite3_close(db);
        exit(1);
    }
    vf = sqlite3_step(res);
    while(vf != SQLITE_DONE)
    {
        if(sqlite3_column_int(res,2) == 0)
        {
            strcat(buffer, "De la ");
            strcat(buffer,sqlite3_column_text(res,0));
            strcat(buffer," ai primit mesajul:\n");
            strcat(buffer,sqlite3_column_text(res,1));
            strcat(buffer,"\n\n");
        }
        else
        {
            strcat(buffer, "De la ");
            strcat(buffer,sqlite3_column_text(res,0));
            strcat(buffer," ai primit raspunsul la mesajul cu id-ul ");
            strcat(buffer,sqlite3_column_text(res,2));
            strcat(buffer," :\n");
            strcat(buffer,sqlite3_column_text(res,1));
            strcat(buffer,"\n\n");
        }
        vf=sqlite3_step(res);
    }
    if(strcmp(buffer,"\nHei! Cat timp ai fost offline ai primit urmatoarele mesaje!\n\n") == 0)
    {
        strcpy(buffer,"\nHei! Cat timp ai fost offline nu ai primit niciun mesaj.\n\n");
    }
    size = strlen(buffer);
    if(write(cl,&size,sizeof(int)) == -1)
    {
        printf("Eroare la write.\n");
        exit(1);
    }
    if(write(cl,buffer,size) == -1)
    {
        printf("Eroare la write.\n");
        exit(1);
    }
}

void set_online(char * nume_utilizator)
{
    int dc;
	char query[100]="";
	strcat(query, "UPDATE utilizatori set status = 'online' where name = '");
	strcat(query, nume_utilizator);
	strcat(query,"'");
	dc =  sqlite3_exec(db, query, 0, 0, &err_msg);
	if (dc != SQLITE_OK )
	{
		printf("Eroare la inserare in bd!\n");        
		exit(1);
	}
}

void set_offline(char * nume_utilizator)
{
    int dc;
	char query[100]="";
	strcat(query, "UPDATE utilizatori set status = 'offline' where name = '");
	strcat(query, nume_utilizator);
	strcat(query,"'");
	dc =  sqlite3_exec(db, query, 0, 0, &err_msg);
	if (dc != SQLITE_OK )
	{
		printf("Eroare la inserare in bd!\n");        
		exit(1);
	}
}

void afisare_conversatie(int cl, char *nume_utilizator, char *nume_prieten)
{
    sqlite3_stmt *res;
    int vf;
    char user[30];
    char sql[100]="SELECT id, sender,text from messages where receiver ='";
    strcat(sql,nume_utilizator);
    strcat(sql,"' and sender ='");
    strcat(sql,nume_prieten);
    strcat(sql,"' OR receiver ='");
    strcat(sql,nume_prieten);
    strcat(sql,"' AND sender = '");
    strcat(sql,nume_utilizator);
    strcat(sql,"'");
    vf = sqlite3_prepare_v2(db,sql,-1,&res,0);
    if(vf != SQLITE_OK)
    {
        printf("Eroare la interogare.\n");
        sqlite3_close(db);
        exit(1);
    }
    vf = sqlite3_step(res);
    char buffer[1000]="mesaje:Priveste conversatia ta cu ";
    strcat(buffer,nume_prieten);
    strcat(buffer,"\n");
    int dim = strlen(buffer);
    while(vf != SQLITE_DONE)
    {
        sprintf(user,"%d",sqlite3_column_int(res,0));
        strcat(buffer,user);
        strcat(buffer,"  ");
        strcat(buffer,sqlite3_column_text(res,1));
        strcat(buffer," ");
        strcat(buffer,":");
        strcat(buffer," ");
        strcat(buffer,sqlite3_column_text(res,2));
        strcat(buffer,"\n");
        vf=sqlite3_step(res);
    }

    printf("%s",buffer);
    
    if(strlen(buffer) == dim)
    {
        strcpy(buffer,"mesaje:Nu exista mesaje cu acest utilizator.\n");
    }

    size = strlen(buffer);
    if(write(cl,&size,sizeof(int)) == -1)
    {
        printf("Eroare la write.\n");
        exit(1);
    }
    if(write(cl,buffer,size) == -1)
    {
        printf("Eroare la write.\n");
        exit(1);
    }
}
