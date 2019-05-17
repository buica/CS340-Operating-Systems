/*******************************************
* Calvin Bui
* CS 344 
* Program 2 - Adventure
* Buildrooms program
********************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <fcntl.h>

/* Set global variables/constants and other initializations up top here*/
#define ROOM_LIMIT 7   /* Only 7 of the 10 total rooms will be used per run */
#define false 0
#define true 1

/* room struct */ 
struct room 
{
	char *name;
	char *type;
	int connectionCount;  /* keep count of outbound connections a room has. Should not exceed 6. */
	char *connections[6]; /* Room array to hold connected outbound rooms in. Should hold 6 rooms */
};

/* function declarations */
int IsGraphFull(struct room*);  
int canAddConnectionFrom(struct room*);
int alreadyUsed(int, int*, int);
void addRandomConnection(struct room*);
int connectionAlreadyExists(struct room*, int, int);
void printRooms(struct room*);
void makeRoomFiles(struct room *roomArray);

int main()
{
	srand(time(NULL)); /* Use current time as seed for random room generator */
	/* Initialize Room Array to store our rooms in */
	struct room roomArray[ROOM_LIMIT];

	/* Create 10 hard-coded unique room names to be used in-game */
	char *ROOM_NAME[10];
	ROOM_NAME[0] = "yharnam";
	ROOM_NAME[1] = "ward";
	ROOM_NAME[2] = "hemwick";
	ROOM_NAME[3] = "lamp";
	ROOM_NAME[4] = "dungeon";
	ROOM_NAME[5] = "workshop";
	ROOM_NAME[6] = "yahargul";
	ROOM_NAME[7] = "sickroom";
	ROOM_NAME[8] = "mensis";
	ROOM_NAME[9] = "church";

	/* Initialize and generate room array */
	int randomIndex, i;  /* Index for drawing room names randomly from our hard-coded list of 10 */
	int drawnNums[ROOM_LIMIT]; /* Array to keep track of drawn rooms by their index number */
	
	for (i = 0; i < ROOM_LIMIT; i++)
	{
		randomIndex = rand() % 10;
		/* check to see if number has been already drawn and redraw if so*/
		while (alreadyUsed(randomIndex, drawnNums, i) == true)
		{
			randomIndex = rand() % 10;  
		}
		drawnNums[i] = randomIndex;
		roomArray[i].name = ROOM_NAME[randomIndex]; /* assign drawn room name to indexed room element in array */
		roomArray[i].connectionCount = 0; /* Initialize number of outbound connections to 0 */ 

		/* Assign room type depending on position of room in array */
		if (i == 0)
			roomArray[i].type = "START_ROOM";
		else if (i == 6)
			roomArray[i].type = "END_ROOM";
		else
			roomArray[i].type = "MID_ROOM";
	}

	/* Create all connections in graph */
	while (IsGraphFull(roomArray) == false)
	{
		addRandomConnection(roomArray);
	}
	/*printRooms(roomArray);*/
    makeRoomFiles(roomArray); /* make our directory and room files for adventure.c to read from */

	return 0;
}

/* Helper function to print out rooms and test my code */
void printRooms(struct room* roomArray)
{
	int i, j;
	for (i = 0; i < ROOM_LIMIT; i++)
    {
		printf("Room: %s\n", roomArray[i].name);
		printf("Has: %d outbound connections\n", roomArray[i].connectionCount);
	    /* print out all connections for each room */
	    if (roomArray[i].connectionCount > 0)
	    {
            int count = roomArray[i].connectionCount;
		    for (j = 0; j < count; j++)
			    printf("Connection %d: %s\n", j+1, roomArray[i].connections[j]);
        }
        printf("Room Type: %s\n", roomArray[i].type);
    }
	printf("\n");
}

/* Returns true if all rooms have 3-6 connections, returns false otherwise */
int IsGraphFull(struct room* roomArray)  
{
  int i;
  for (i = 0; i < ROOM_LIMIT; i++)
  {
  	if (roomArray[i].connectionCount < 3)
  		return false;
  }
  return true;
}

/* Return true if room x has less than 6 outbound connections, returns false otherwise */
int canAddConnectionFrom(struct room* room)
{
	if (room->connectionCount < 6)
		return true;
	else
		return false;
}

/* Scans through index in range max, returns true if we already used the same room name, returns false otherwise */
int alreadyUsed(int i, int* drawnNums, int max)
{
	int k;
	for (k = 0; k < max; k++)
	{
		if (drawnNums[k] == i)
			return true;
	}
	return false;
}

/* Draws two random room names, then connects them to each other */
void addRandomConnection(struct room* roomArray)
{
	int randomA, randomB; /* ints of room indices are easier to work with than room structs for this function */

	while (true)
	{
		randomA = rand() % 7;  /* get first random room's index */
        if (canAddConnectionFrom(&roomArray[randomA]) == true) /* if we can add another connection from A, break out */	
            break;
    } 
	
	do  
	{
		randomB = rand() % 7;  /* Get our second random room's index */
	}while (connectionAlreadyExists(roomArray, randomA, randomB) == true ||
			canAddConnectionFrom(&roomArray[randomB]) == false ||  
			randomA == randomB); /* keep drawing a new room index til we get one we can connect to A */

	/* connect rooms together */
	int a = roomArray[randomA].connectionCount; /* store values of room connections */
	int b = roomArray[randomB].connectionCount;
	roomArray[randomA].connections[a] = roomArray[randomB].name; /* assign rooms to each other's connections */
	roomArray[randomB].connections[b] = roomArray[randomA].name;
	roomArray[randomA].connectionCount++;
	roomArray[randomB].connectionCount++;
}

/* If a connection between room a and room b already exists, return true. Return false otherwise */
int connectionAlreadyExists(struct room* roomArray, int indexA, int indexB)
{
	int i;
	for (i = 0; i < roomArray[indexA].connectionCount; i++)
	{
		if (roomArray[indexA].connections[i] == roomArray[indexB].name)
		{
			return true;
		}
	}
	return false;
}

/* creates a unique directory then creates 7 room files to put into the directory. 
    Room content is then written into each corresponding file */
void makeRoomFiles(struct room *roomArray)
{
	/* create our directory and files, then write room data to files and store in directory */
	int pid = getpid(); /* process id for appending to directory */
	char directory[32];
	sprintf(directory, "buica.rooms.%i", pid); /* attach pid to end of our directory name */
	int results = mkdir(directory, 0755); /* create directory with unique pid */
	FILE *fileDescriptor;
	char readBuffer[32];
    /* these 3 strings are needed in order to format our file content correctly */
    char *roomNameLabel = "ROOM NAME: ";
    char *roomConnectionLabel = "CONNECTION ";
    char *roomTypeLabel = "ROOM TYPE: ";
    /* create room files for all 7 generated rooms inside our buica.rooms.(pid) directory we just created */
    char newFilePath[32];
    int r; /* int that we will use for looping through and creating all our room files */
    
	for (r = 0; r < ROOM_LIMIT; r++)
    {
        sprintf(newFilePath, "%s/room%d.txt", directory, r+1); /* set the file path we want to open */
        fileDescriptor = fopen(newFilePath, "w"); /* open filestream for writing */
		if (fileDescriptor == NULL) {printf("HULL BREACH - fopen() failed on \"%s\"\n", newFilePath); exit(1);}
		memset(newFilePath, '\0', sizeof(newFilePath)); /* reset newFilePath to store next file name */
    
        /* print data to our room files */
        fprintf(fileDescriptor, "ROOM NAME: %s\n", roomArray[r].name);
        
        int cCount = roomArray[r].connectionCount;
        int j;
        for (j = 0; j < cCount; j++)
            fprintf(fileDescriptor, "CONNECTION %d: %s\n", j+1, roomArray[r].connections[j]);
        fprintf(fileDescriptor, "ROOM TYPE: %s\n", roomArray[r].type);
    }
    fclose(fileDescriptor);
}

