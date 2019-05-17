/***************************************************************
 * Calvin Bui
 * CS 340 Operating Systems I
 * Program 2 - adventure
 * Adventure program
 * *************************************************************/
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>

/* Set global variables/constants and other initializations up top here*/
#define ROOM_LIMIT 7   /* Only 7 of the 10 total rooms will be used per run */
#define false 0
#define true 1

static pthread_mutex_t myMutex = PTHREAD_MUTEX_INITIALIZER; /* initialize our mutex for unlocking/locking thread later */
/* this version of our room struct has char arrays instead of pointers, so we can use read/write */ 
struct room 
{
	char name[32];
	char type[32];
	int connectionCount;  /* keep count of outbound connections a room has. Should not exceed 6. */
	char connections[6][32]; /* Room array to hold connected outbound rooms in. Should hold 6 rooms */
};

void *printTime(void *arg); /* Threading implementation starts at line 157 and the locking/unlocking is in the function itself */

int main()
{
	/* we will put contents of opened files back into array of room structs */
	struct room roomArray[ROOM_LIMIT];

	/* Search through our current direcotry to find the most recent rooms directory */
    /* CITATION: taken from readDirectory.c example file in 2.4 */
	int newestDirTime = -1; // The modified timestamp of the newest subdir we've examined
	char targetDirPrefix[32] = "buica.rooms."; // The prefix we're looking for for all subdirs
	char newestDirName[256]; // Holds the name of the newest directory that contains the prefix
	memset(newestDirName, '\0', sizeof(newestDirName));

	DIR* dirToCheck; // Holds the directory we're starting in
	struct dirent *fileInDir; // Holds the current subdir of the starting dir we're looking at
	struct stat dirAttributes; // Holds information we've gained about subdir
	dirToCheck = opendir("."); // Open up the directory this program was run in

	if (dirToCheck > 0) // Make sure the current directory could be opened
	{ 
		while ((fileInDir = readdir(dirToCheck)) != NULL) // Check each entry in the directory
		{
			if (strstr(fileInDir->d_name, targetDirPrefix) != NULL) // If entry has the prefix
			{
		        stat(fileInDir->d_name, &dirAttributes); // Get attributes of the entry
				
				if ((int)dirAttributes.st_mtime > newestDirTime) /* if entry's attributes are newer */
				{
					newestDirTime = (int)dirAttributes.st_mtime;	/* assign time of newer directory */
					memset(newestDirName, '\0', sizeof(newestDirName));
					strcpy(newestDirName, fileInDir->d_name); /* copy over newer entry's data */
					//printf("Newer subdir: %s, new time: %d\n", fileInDir->d_name, newestDirTime);
				}
			}
		}
	}
	closedir(dirToCheck); /* close the directory after we're finished with it */

	/* open up and read the contents from the files */
	FILE *fileDescriptor;
	char readBuffer[32]; /* initialize our buffers for file-reading and writing */
	char filePath[32];

	/* loop through all our files and assign the room, connections, and types to our new roomArray */
	int i = 1;
	for (i = 1; i <= ROOM_LIMIT; i++)
	{	
		sprintf(filePath, "%s/room%d.txt", newestDirName, i); /* get the whole filepath name */
		fileDescriptor = fopen(filePath, "r"); /* open file for reading */
		if (fileDescriptor == NULL) {printf("HULL BREACH - fopen() failed on \"%s\"\n", filePath); exit(1);}
		
		/* extract the room name, connection names, and type from each file we get, and assign them to rooms in our roomArray */
		fscanf(fileDescriptor, "%*s %*s %s\n", roomArray[i - 1].name); /* scan file stream, and extract the room name */
		//printf("room %d: %s\n", i, roomArray[i - 1].name);

		/* use a while loop to get all our room connections 
			fscanf returns the amount of scanned values if it succeeds 
			continue to assign*/
		int count = 0; /* keep track of our row count */
		int scannedValue = 0; /*we need this throwaway value for us to differentiate between CONNECTION and ROOM TYPE rows */ 
		while (fscanf(fileDescriptor, "%*s %d: %s", &scannedValue, roomArray[i - 1].connections[count]) == 2)
		{
			//printf("connection %d: %s\n", count+1, roomArray[i - 1].connections[count]);
			count++; /* get the total connection count so we can get them all in a single for loop*/
		}		
		/* assign count to each rooms connectionCount and reset for next iteration */
		roomArray[i -1].connectionCount = count;
		//printf("total connections: %d\n", count);
		count = 0;
		
		/* room 1 is always START room, and room 7 is always END room, so we can assign types manually */
		if (i - 1 == 0)
			strcpy(roomArray[i - 1].type, "START_ROOM");
		else if (i - 1 == 6)
			strcpy(roomArray[i - 1].type, "END_ROOM");
		else
			strcpy(roomArray[i - 1].type, "MID_ROOM");

		//printf("type: %s\n", roomArray[i - 1].type);
	}
	
	
	/* Game Implementation */
    /* prompt user for input and get input line from them */
	int typeCounter = 0; /* increment this value to help keep track of our room type */
	/* CITATION: Used 2.4 File Access in C as a basis for this implementation */
	int numCharsEntered = -5; /* track how many chars we entered */
	int currChar = -5;  /* track where we are when we print out all chars */
	size_t bufferSize = 0;
	char *lineEntered = NULL; /* max: 32 chars */

	struct room *currentRoom; /* initialize current game struct to keep track of player progression */
	currentRoom = &roomArray[0]; /* Set the start room as current room */
	char *visitedRooms[50]; /* pointer to char array for holding list of visited rooms (50 max) */
	int steps; /* keep track of steps a player has taken */
	int prevSteps; /* for comparing to steps value. Only way I caould think of for telling users their input is bad */

	/* keep looping until player reaches the end room */
	while(strcmp(currentRoom->type, "END_ROOM") != 0)
	{
		printf("\nCURRENT LOCATION: %s\n", currentRoom->name);
		printf("POSSIBLE CONNECTIONS: ");
		/* list possible rooms for player to enter */
		int j; 
		for (j = 0; j < currentRoom->connectionCount; j++)
		{	
			if (j < currentRoom->connectionCount - 1)
				printf("%s, ", currentRoom->connections[j]);
			else
				printf("%s.\n", currentRoom->connections[currentRoom->connectionCount - 1]);
		}

		printf("WHERE TO? >");
		numCharsEntered = getline(&lineEntered, &bufferSize, stdin); /* get initial input */
		lineEntered[numCharsEntered - 1] = '\0'; /* get rid of newline */

		/* wait for user to call time function */
		if (strcmp(lineEntered, "time") == 0)
		{
			pthread_t timeThread; /* create our thread for executing time function */
			int result_code, thread_args;
			 /* create our thread and call the printTime function as its starting routine */
			result_code = pthread_create(&timeThread, NULL, printTime, (void *) &thread_args);
			assert(0 == result_code); 

			pthread_join(timeThread, NULL); /* wait for thread to finish, then rejoin main thread */
		}
		
		/* loop through all connections to see if user input matches any conecting rooms */
		int k; 
		for (k = 0; k < currentRoom->connectionCount; k++)
		{
			if  (strcmp(lineEntered, currentRoom->connections[k]) == 0) /* if player's input matches one of the connectons */
			{
				int m;
				for (m = 0; m < ROOM_LIMIT; m++)
				{
					if (strcmp(roomArray[m].name, lineEntered) == 0) /* find matching room in roomArray */
					{
						currentRoom = &roomArray[m]; /* set entered room name to new current room */
						visitedRooms[steps] = roomArray[m].name; /* add new current room's name to list of visited rooms */
					}	
				}
				steps++; /* increment total steps each time we enter a valid room */
			}

		}
		
		//pthread_join(thread, NULL);
	}

	/* print out the endgame message and show game stats */
	printf("\nYOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n"); 
	printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", steps);
	int n;
	for (n = 0; n < steps; n++)
	{
		printf("%s\n", visitedRooms[n]);
	}

    return 0;
}

/* print out time to stdout and also print it to a txt file */
/*CITATION: Used this site to learn about the library function localtime();
	source: https://www.tutorialspoint.com/c_standard_library/c_function_localtime.htm */
void *printTime(void *arg)
{
	pthread_mutex_lock(&myMutex); /* acquire the lock so this thread runs exclusively */
	
	time_t rawtime;
	struct tm *timeinfo;
	time( &rawtime );
	timeinfo = localtime ( &rawtime ); /* get local time */
	char timeString[64];

	/* format our time string correctly ( like this: 9:09pm, Friday, May 10, 2019 \n\n)*/
	strftime(timeString, sizeof(timeString), "%r, %A, %B %e, %Y\n", timeinfo);
	printf("\n %s \n", timeString);

	/* print our current time to a file called currentTime.txt */
	FILE *timeFileDescriptor;
	timeFileDescriptor = fopen("currentTime.txt", "w"); /* open filestream for writing */
	fprintf(timeFileDescriptor, "%s", timeString); /* print time to file (couldn't figure out how to print it before program exits) */

	/* now we can unlock the thread again and return to the main thread */
	pthread_mutex_unlock(&myMutex);
	
}
	
	