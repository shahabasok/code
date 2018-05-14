# code
This application is verified in Ubuntu-16.04
Tried to use POSIX standard for better compatibility. Need to install sqlite3 for running this application.
The code can be compiled using the following command:
    #gcc -o reminder.c -l sqlite3 -l pthread

Basic architecture:
    This application relies on the alarm() API to track the immediate reminder. Using the alarm() API, a call back will be provided per process when this timer expires. The existing timer can also be canceled if zero is passed to alarm API as an argument. Every time a reminder is added to the DB, the DB will be sorted out based on the date and time in ascending order. The top entry of the list will be taken and its remaining seconds are calcuted. This will be set to the alarm(). If there any active timer is running, then that will be canceled befoe this operation. When the alarm expires, a new thread will be spawned to ensure that the alarm's call back is not blocked for long time. This thread will ensure the expired entry is removed from the DB and take the next most date/time to set the alarm. Checks are done to see if there any reminders set for past. 
     The entire logic is under one process, but the plan was to split in to two modules and processes. One process will front-end to verify the data set by the user and the another module will run in background all the time and will set the alarm accordingly. Both the processes can be connected via pipe to ensure the sync between them. There was also plan to use GTK for receiving the user input and to give pop on timer expiry.
     This application is desinged to handle multiple reminders set at same time. But not verified this as of now.
     The measures are taken to take the next immediate event quikly and to reschedule the entries  
