#define _POSIX_SOURCE
#include <stdio.h>
#include <sqlite3.h> 
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>

#define DB_PATH "/opt/reminders/reminders.db"
char alarm_set_time[20];
volatile int footprint=0;
int set_alarm(unsigned int sec);

struct pair
{
	char in_time[25];
	char msg[215];
};

static int callback_generic(void *NotUsed, int argc, char **argv, char **azColName) {
	int i;
	puts("generic");
	for(i = 0; i<argc; i++) {
	  printf("callback_generic: %s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	return 0;
}
static int callback_sort(void *NotUsed, int argc, char **argv, char **azColName) {
	int i;
	printf("%s: argc= %d\n",__FUNCTION__, argc);
	for(i = 0; i<argc; i++) {
	  printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	  	if(0 == strcmp(azColName[i],"DATE"))
	{
		long in_seconds = 0;
		if(0 == remaining_seconds(argv[i], &in_seconds))
		{
			memset(alarm_set_time,0,20);
			strcpy(alarm_set_time,argv[i]);
			set_alarm(in_seconds);
			printf("Alarm set successfully\n");
		}
	}
	}
	printf("\n");
	return 0;
}


void *popup_handler(void *arg)
{
	struct pair *p = (struct pair*)arg;
	printf("%s: date = %s, msg = %s\n",__FUNCTION__, p->in_time,p->msg);
	free(p);
	return NULL;
}

static int callback_alarmHandler(void *NotUsed, int argc, char **argv, char **azColName) {
   int i;
   printf("%s\n",__FUNCTION__);
	struct pair *p=(struct pair *)malloc(sizeof(struct pair));
	strcpy(p->in_time,alarm_set_time);
		
   for(i = 0; i<argc; i++) {
		if(0 == strcmp("MESSAGE",azColName[i]))
		{
			strcpy(p->msg,argv[i]);
		}
		printf("callback_alarmHandler: %s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
	pthread_t thread_id; 
	pthread_create(&thread_id, NULL, popup_handler, p);
	sleep(0); 
   printf("\n");
   return 0;
}


int first_time(char *path)
{
	sqlite3 *db;
	struct stat buf;
	char *sql;
	char *zErrMsg = 0;
   	int rc;
	
	printf("%s\n",__FUNCTION__);
	if(0 == stat(path,&buf) )
	{
		return 0;
	}
   rc = sqlite3_open(path, &db);

   if( rc ) 
	{
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return(-1);
   	} else 
	{
      fprintf(stderr, "Opened database successfully\n");
   }


   	/* Create SQL statement */
   	sql = "CREATE TABLE REMINDER("  \
        	 "ID INTEGER PRIMARY KEY     AUTOINCREMENT," \
         	"DATE           DATETIME    NOT NULL," \
		"MESSAGE	CHAR(215)	NOT NULL );";
		
   	/* Execute SQL statement */
   	rc = sqlite3_exec(db, sql, callback_generic, 0, &zErrMsg);
   
   	if( rc != SQLITE_OK )
	{
   		fprintf(stderr, "SQL error: %s\n", zErrMsg);
      		sqlite3_free(zErrMsg);
   	} 
	else 
	{
      		fprintf(stdout, "Table created successfully\n");
  	}	
   	sqlite3_close(db);
	return 0;	
}

void *alarm_handler(void *arg)
{
	sqlite3 *db;
	struct stat buf;
	char *zErrMsg = 0;
	int rc;		
	char sql[400] = {0}; 
	char d_alarm_set_time[25] = {0}; 
	printf("%s\n",__FUNCTION__);
	char *query = "SELECT * FROM REMINDER WHERE DATE ='";
	sprintf(sql, "%s%s\'", query, alarm_set_time);
	strcpy(d_alarm_set_time,alarm_set_time);
	rc = sqlite3_open(DB_PATH, &db);
	if( rc ) 
	{
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
	} 
	else 
	{
		fprintf(stderr, "Opened database successfully\n");
	}
	rc = sqlite3_exec(db, sql, callback_alarmHandler, 0, &zErrMsg);
	if( rc != SQLITE_OK)
	{
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	else
	{
		fprintf(stdout, "alarm handler exited\n");
	}
	memset(sql,0,400);
	query = "DELETE FROM REMINDER WHERE DATE ='";
	printf("%s:deleting entry: %s",__FUNCTION__,d_alarm_set_time);
	sprintf(sql, "%s%s\'", query, d_alarm_set_time);
	rc = sqlite3_exec(db, sql, callback_generic, 0, &zErrMsg);
        if( rc != SQLITE_OK)
        {
                fprintf(stderr, "SQL error: %s\n", zErrMsg);
                sqlite3_free(zErrMsg);
        }
        else
        {
                fprintf(stdout, "Deletd the entry\n");
        }
	puts("calling sort_by_date again");
	memset(alarm_set_time,0,20);
	sort_by_date();
	sqlite3_close(db);
	return 0;
}

void catcher(int signum) {
  pthread_t thread_id; 
  
  printf("%s\n",__FUNCTION__);
  puts("inside signal catcher!");
  footprint = 1;
  pthread_create(&thread_id, NULL, alarm_handler, NULL);
  sleep(0);
}

unsigned int reset_alarm()
{
	return alarm(0);
}




/* expected format is Month date hrs:minutes:seconds year*/
int remaining_seconds(char *time_format, long *diff_seconds) 
{
	struct tm tm;
	time_t t;
	if(diff_seconds == NULL)
	{
		return -3;
	}
	if(NULL != strptime(time_format, "%Y-%m-%d %H:%M:%S", &tm) )
	{
		tm.tm_isdst = -1;
	}
	else
	{
		puts("Invalid format");
		return -1;
	}
	t = mktime(&tm);
	*diff_seconds = (long) t - (long)time(NULL);
	if(0 >= *diff_seconds)
	{
		puts("Time is in the past");
		return -2;
	}	
	printf("seconds since the Epoch: %ld\n", *diff_seconds);
	return 0;
}

int set_alarm(unsigned int sec)
{
  struct sigaction sact;
  volatile double count;
  time_t t;
  printf("%s\n",__FUNCTION__);

  sigemptyset(&sact.sa_mask);
  sact.sa_flags = 0;
  sact.sa_handler = catcher;
  sigaction(SIGALRM, &sact, NULL);

  alarm(sec); 

/*
  time(&t);
  printf("before loop, time is %s", ctime(&t));
  for (count=0; (count<1e10) && (footprint == 0); count++);
  time(&t);
  printf("after loop, time is %s", ctime(&t));

  printf("the sum so far is %.0f\n", count);

  if (footprint == 0)
    puts("the signal catcher never gained control");
  else
    puts("the signal catcher gained control");
*/
}

reschedule_alarms()
{

	/*take the top most from DB*/
	/* set alarm */
}

int update_db(char *date_c, char *msg, int id)
{
	sqlite3 *db;
        struct stat buf;
        char *zErrMsg = 0;
        int rc;
	char sql[400] = {0};
	printf("%s\n",__FUNCTION__);
	char query[100] = "UPDATE REMINDER SET";
	if((0 != strlen(date_c)) && (0 != strlen(msg)))
	{
		sprintf(sql, "%s DATE = \'%s\'\, MESSAGE = \'%s\' WHERE ID = %d", query,date_c,msg,id);
	}
	else if((0 != strlen(date_c)) && (0 == strlen(msg)))
	{
		sprintf(sql, "%s DATE = \'%s\'\  WHERE ID = %d",query,date_c,id);
	}
	else if((0 != strlen(date_c)) && (0 == strlen(msg)))
	{
		sprintf(sql, "%s MESSAGE = \'%s\'\  WHERE ID = %d",query,msg,id);
	}
	else
	{
		return 1;
	}	

	rc = sqlite3_open(DB_PATH, &db);
   	if( rc ) 
	{
      		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
   	} 
	else 
	{
      		fprintf(stderr, "Opened database successfully\n");
   	}
	rc = sqlite3_exec(db, sql, callback_sort, 0, &zErrMsg);
	if( rc != SQLITE_OK)
	{
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	else
	{
		fprintf(stdout, "Date is sorted\n");
	}
	sqlite3_close(db);

	return 0;

}
int sort_by_date()
{
        sqlite3 *db;
        struct stat buf;
        char *zErrMsg = 0;
        int rc;
	
	printf("%s\n",__FUNCTION__);
	char sql[400] = "SELECT * from REMINDER ORDER BY DATE ASC LIMIT 0, 1 ";

	rc = sqlite3_open(DB_PATH, &db);
   	if( rc ) 
	{
      		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
   	} 
	else 
	{
      		fprintf(stderr, "Opened database successfully\n");
   	}
	rc = sqlite3_exec(db, sql, callback_sort, 0, &zErrMsg);
	if( rc != SQLITE_OK)
	{
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	else
	{
		fprintf(stdout, "Date is sorted\n");
	}
	sqlite3_close(db);
	return 0;
}

int add_to_db(char *in_time, char *message)
{
        sqlite3 *db;
        struct stat buf;
        char *zErrMsg = 0;
        int rc;
	
	char sql[400] = {0};
	printf("%s\n",__FUNCTION__);

	char *const_val = "INSERT INTO REMINDER (DATE, MESSAGE) VALUES ('";
        sprintf(sql, "%s%s\'\, \'%s\'\)\; ", const_val, in_time, message);

	rc = sqlite3_open(DB_PATH, &db);
   	if( rc ) 
	{
      		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
   	} 
	else 
	{
      		fprintf(stderr, "Opened database successfully\n");
   	}
	rc = sqlite3_exec(db, sql, callback_generic, 0, &zErrMsg);
	if( rc != SQLITE_OK)
	{
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	else
	{
		fprintf(stdout, "Data inserted\n");
	}
	sqlite3_close(db);
	return 0;
}
int list_all()
{
        sqlite3 *db;
        struct stat buf;
        char *zErrMsg = 0;
        int rc;
	
	char sql[400] = "SELECT * FROM REMINDER";

	rc = sqlite3_open(DB_PATH, &db);
   	if( rc ) 
	{
      		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
   	} 
	else 
	{
      		fprintf(stderr, "Opened database successfully\n");
   	}
	rc = sqlite3_exec(db, sql, callback_generic, 0, &zErrMsg);
	if( rc != SQLITE_OK)
	{
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	else
	{
		fprintf(stdout, "Data inserted\n");
	}
	sqlite3_close(db);

	return 0;
}
int main()
{
	first_time(DB_PATH);
	while(1)
	{
	//set_alarm(6);
	long l = 0;
	char selection[3] = {0};
	//remaining_seconds("2018-05-14 08:43:50", &l);
	//set_alarm(l);
	puts("Please enter the below options, enter 'c' anytime to comeback to this menu");
	puts("1. Set reminder");
	puts("2. Update reminder");
	puts("3. List reminder");
	puts("4. Remove reminder");
	puts("5. Quit");
	fgets(selection,3,stdin);
	printf("input: [%s]\n",selection);
	if(2 != strlen(selection))
	{
		puts("Invalid entry");
		continue;
	}
	switch(selection[0])
	{
		case '1':
		{
			while(1)
			{
				long seconds = 0;
				char in_time[25]= {0};/* actually 19 required, just to catch user input issue*/
				char message[202] = {0};
				
				puts("Set reminder text less than 200 words");
				fgets(message,200,stdin);

				if(0 == strcmp("c\n",in_time))
				{
					puts("Going to Main Menu...!!\n");
					break;
				}
			
				puts("Set reminder time in yyyy-mm-dd hh:mm:ss format");

				fgets(in_time,25,stdin);
				//printf("string = [%s]\n",in_time);	
				if(0 == strcmp("c\n",in_time))
				{
					puts("Going to Main Menu...!!\n");
					break;
				}
				if(20 != strlen(in_time)) /* counting new line */
				{
					printf("Invalid format, please retry = %d\n",strlen(in_time));
					continue;
				}
				in_time[19] = 0;/*trim new line*/
				if( 0 != remaining_seconds(in_time,&seconds))
				{
					continue;
				}
				alarm(0);
				add_to_db(in_time,message);
				sort_by_date();
				break;
			}
		}	
		break;
		
		case '2':
			while(1)
			{	
				char id[10] = {0};
				char date_c[25] = {0};
				char msg[225] = {0};
				puts("Enter the ID number to update");
				fgets(id,10,stdin);
				if(0 == strcmp("c\n",id))
				{
					puts("Going to Main Menu...!!\n");
					break;
				}
				int i = atoi(id);
				puts("Enter date to be modified, to skip enter 's'");
				fgets(date_c,25,stdin);
                                if(0 == strcmp("s\n",date_c))
                                {
                                        date_c[0] = 0;
                                }
				puts("Enter message to be modified, to skip enter 's'");
				fgets(msg,210,stdin);
                                if(0 == strcmp("s\n",msg))
                                {
                                        msg[0] = 0;
                                }	
				update_db(date_c,msg,i);
				break;
			}		
		break;
		case '3':
			list_all();
			
		break;
		case '4':
			/*TBD*/
		break;
		case '5':
			puts("Bye...!!");
			return 0;
		break;
	}


#if 0
  	while(1)
	{
		sleep(3);
	} 
#endif
	}
}


/* return 0 if same, return 1 if A>B, -1 if B>A */
int compare_time(char *timeA, char*timeB)
{
        struct tm tmA, tmB;
        time_t tA, tB;
        if(strptime(timeA, "%Y-%m-%d %H:%M:%S", &tmA) != NULL)
        {
                tmA.tm_isdst = -1;
        }
        else
        {
                puts("Invalid format timeA");
                return -1;
        }
        tA = mktime(&tmA);

        if(strptime(timeB, "%Y-%m-%d %H:%M:%S", &tmB) != NULL)
        {
                tmB.tm_isdst = -1;
        }
        else
        {
                puts("Invalid format timeB");
                return -1;
        }
        tB = mktime(&tmB);

	if((long)tA > (long)tB) return 1;
	if((long)tB > (long)tA) return -1;
	if((long)tB == (long)tA) return 0;
}


