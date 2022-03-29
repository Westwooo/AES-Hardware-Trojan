#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define END 0
#define SPACE 32
#define DASH 45
#define FULLSTOP 46
#define C 67
#define I 73
#define L 76
#define M 77

char startLocations[200][20];
char startPorts[200][25];
char targetLocations[200][25];
char targetPorts[200][25];

char locationSearchFront[200][100][30];
char portSearchFront[200][100][30];
int frontSize[200];

FILE * pathSearchScript;

//A method to extract the Cartesian coordinates of a tile
void getTileCoords(char tile[], int* x, int* y) {
	
	int yCoordFound = 0;
	int xIndex, yIndex = 0;
	char xCoordString[4], yCoordString[4];
	
	//Extract the X and Y coordinates from the tile name
	for(int i = 0; i < strlen(tile); i++) {
		
		if(tile[i] == 88) {
		
			i++;
			int xIndex = i;
			
			while(tile[i] != 89) {
				xCoordString[i - xIndex] = tile[i];
				i++;
			}
			
			xCoordString[i-xIndex] = 0;
			*x = atoi(xCoordString);
			
			yCoordFound = 1;

			i++;
			yIndex = i;
		}
		
		if(yCoordFound == 1) {
			yCoordString[i - yIndex] = tile[i];
		}
	}
	yCoordString[strlen(tile) - yIndex] = 0;
	*y = atoi(yCoordString);
}

//Print the command to search for a path in goahed to the file - pathSearchScript
void fputPaths(char startPort[], char startLocation[], char targetPort[], char targetLocation[], char printBanner[]) {

	fputs("PathSearchOnFPGA", pathSearchScript);
	fputs(" BlockUsedPorts=True", pathSearchScript);
	fputs(" OutputMode=CHAIN", pathSearchScript);
	fputs(" StartLocation=", pathSearchScript);
	fputs(startLocation, pathSearchScript);
	fputs(" StartPort=", pathSearchScript);
	fputs(startPort, pathSearchScript);
	fputs(" TargetLocation=", pathSearchScript);
	fputs(targetLocation, pathSearchScript);
	fputs(" TargetPort=", pathSearchScript);
	fputs(targetPort, pathSearchScript);
	fputs(" MaxSolutions=1", pathSearchScript);
	fputs(" MaxDepth=40", pathSearchScript);
	fputs(" PrintBanner=", pathSearchScript);
	fputs(printBanner, pathSearchScript);
	fputs(" CreateBackupFile=True", pathSearchScript);
	fputs(" ExactPortMatch=True", pathSearchScript);
	fputs(" FileName='C:\\\\Users\\\\Jack\\\\Desktop\\\\FPGA Trojan\\\\foundPaths.txt';", pathSearchScript);
	//fputs(" FileName='C:\\Users\\Jack\\Desktop\\FPGA Trojan\\pathTest.txt';", pathSearchScript);
	fputc(10, pathSearchScript);

}

void fputBlock(char port[], char location[]) {
	
	fputs("SetPortUsageInSelection PortName=", pathSearchScript);
	fputs(port, pathSearchScript);
	fputs(" CheckForExistence=False IncludeReachablePorts=True PortUsage=Blocked TileLocation=", pathSearchScript);
	fputs(location, pathSearchScript);
	fputs(" TilesToExclude=;", pathSearchScript);
	fputc(10, pathSearchScript);
	
}

void fputUnblock(char port[], char location[]) {
	fputs("SetPortUsageInSelection PortName=", pathSearchScript);
	fputs(port, pathSearchScript);
	fputs(" CheckForExistence=False IncludeReachablePorts=True PortUsage=Free TileLocation=", pathSearchScript);
	fputs(location, pathSearchScript);
	fputs(" TilesToExclude=;", pathSearchScript);
	fputc(10, pathSearchScript);
}

void printPathSearch(int i) {
	
	printf("PathSearchOnFPGA ");
	
	//Probably unecessary
	printf("SearchMode=BFS ");
	printf("Forward=True ");
	printf("KeepPathsIndependent=False ");
	printf("BlockUsedPorts=True ");			//Ports need to be blocked once a path has been found
	printf("OutputMode=CHAIN ");			//TO DO - Choose the best output mode
	printf("StartLocation=%s ", startLocations[i]);
	printf("StartPort=%s ", startPorts[i]);
	printf("TargetLocation=%s ", targetLocations[i]);
	printf("TargetPort=%s;", targetPorts[i]);

	//The start locations and ports - TODO can search from whole search front and find shortest path
	
	
	
	//THe target locations and ports
	
	printf("\n");
}

int main (int argc, char *argv[]) {	

	//FILE * TriggerConnections = fopen("./TriggerConnectionsWithSearchFront.txt", "r");
	FILE * TriggerConnections = fopen("./triggerConnections.txt", "r");
	if(TriggerConnections == NULL) {
		printf("Unable to open Trigger Connections\n");
		exit(EXIT_FAILURE);
	}
	
	pathSearchScript = fopen("./pathSearchScript.tcl", "w");
	if(pathSearchScript == NULL) {
		printf("Unable to open path Search Script\n");
		exit(EXIT_FAILURE);
	}
	
	int locationIndex = 0;

	char * line = NULL;
	char number[3];
	int numberIndex = 0;
	char index;
	size_t len = 0;
	ssize_t read;
	int lineIndex = 0;
	int startIndex = 0;
	
	int testX = 0;
	int testY = 0;
	char xString[4];
	char yString[4];
	char tileTest[5];
	char tempTile[15];

	//Read all lines from the trigger file
	while ((read = getline(&line, &len, TriggerConnections)) != -1) {
		
		printf(line);
		
		/*
		for(int i = 0; i < len; i ++){
			printf("%d, ", line[i]);
		}
		printf("\n");
		*/
	
		//If Line contains a primary trigger connection extract the relevant information
		if(line[0] != SPACE && line[0] != DASH) {
			
			//Initialise the line and start index
			lineIndex = 0;
			startIndex = 0;
			
			//Reach the start of the connection
			while(line[++lineIndex] != C) {}
			startIndex = lineIndex;
			
			//Read in the start location
			while(line[lineIndex] != FULLSTOP) {
				startLocations[locationIndex][lineIndex - startIndex] = line[lineIndex];
				lineIndex++;
			}
			startLocations[locationIndex][lineIndex - startIndex] = 0; 
			lineIndex++;
			startIndex = lineIndex;
			
			//Read in the start port
			while(line[lineIndex] != SPACE) {
				startPorts[locationIndex][lineIndex - startIndex] = line[lineIndex];
				lineIndex++;
			}
			startPorts[locationIndex][lineIndex - startIndex] = 0; 
			
			//Reach the start of the target location
			while(line[++lineIndex] != C) {}
			startIndex = lineIndex;
			
			//Read the target location
			while(line[lineIndex] != FULLSTOP) {
				targetLocations[locationIndex][lineIndex - startIndex] = line[lineIndex];
				lineIndex++;
			}
			targetLocations[locationIndex][lineIndex - startIndex] = 0; 
			lineIndex++;
			startIndex = lineIndex;
			
			//Read the target port
			while(line[lineIndex] != END) {
				targetPorts[locationIndex][lineIndex - startIndex] = line[lineIndex];
				lineIndex++;
			}
			targetPorts[locationIndex][lineIndex - startIndex - 1] = 0;
			
			locationIndex++;
		}
		else if(line[0] == SPACE) {
			
			//Initialise the line and start index
			lineIndex = 0;
			startIndex = 0;
			
			//Reach the start of the connection
			while(line[lineIndex] != C && line[lineIndex] != I) {lineIndex++;}
			startIndex = lineIndex;
			
			//Read in the start location
			while(line[lineIndex] != FULLSTOP) {
				locationSearchFront[locationIndex-1][frontSize[locationIndex-1]][lineIndex - startIndex] = line[lineIndex];
				lineIndex++;
			}
			locationSearchFront[locationIndex-1][frontSize[locationIndex-1]][lineIndex - startIndex] = 0; 
			
			getTileCoords(locationSearchFront[locationIndex-1][frontSize[locationIndex-1]], &testX, &testY);
			memcpy(&tileTest, &locationSearchFront[locationIndex-1][frontSize[locationIndex-1]], 4);
			if(strcmp(tileTest, "CLEM") == 0 && testX > 24) {
				snprintf(tempTile, 20, "CLEM_R_X%dY%d", testX, testY);
				strcpy(locationSearchFront[locationIndex-1][frontSize[locationIndex-1]], tempTile);
			}
			
			lineIndex++;
			startIndex = lineIndex;
			
			//Read in the start port
			while(line[lineIndex] != 10) {
				portSearchFront[locationIndex-1][frontSize[locationIndex-1]][lineIndex - startIndex] = line[lineIndex];
				lineIndex++;
			}
			portSearchFront[locationIndex-1][frontSize[locationIndex-1]][lineIndex - startIndex] = 0;
			//printf("%d, ", lineIndex - startIndex);

			//Initialise the size oif the search front
			frontSize[locationIndex-1]++;
			
		}
    }
	
	//TESTING
	
	char tempPort[21];
	strcpy(tempPort, "CLE_CLE_L_SITE_0_E_O");
	
	//Block all ports that are the start and end of the trigger paths
	//This prevents usage of required connections by earlier paths
	for(int i = 0; i < locationIndex; i++) {
		if(i < 128) {
			fputBlock(portSearchFront[i][1], locationSearchFront[i][1]);
			fputBlock(targetPorts[i], targetLocations[i]);
		}
		else {
			if(startLocations[i][3] == M)
				tempPort[8] = M;
			else
				tempPort[8] = L;
			
			tempPort[17] = startPorts[i][0];
			fputBlock(tempPort, startLocations[i]);
			fputBlock(targetPorts[i], targetLocations[i]);
		}
	}
	
	
	//Print the command to unblock the start and end ports of the path 
	//and print the command to find the path
	for(int i = 0; i < locationIndex; i++) {
		if(i < 128) {
			fputUnblock(portSearchFront[i][1], locationSearchFront[i][1]);
			fputUnblock(targetPorts[i], targetLocations[i]);
			fputPaths(portSearchFront[i][1], locationSearchFront[i][1], targetPorts[i], targetLocations[i], "False");
		}
		else {
			if(startLocations[i][3] == M)
				tempPort[8] = M;
			else
				tempPort[8] = L;
			
			tempPort[17] = startPorts[i][0];
			
			fputUnblock(tempPort, startLocations[i]);
			fputUnblock(targetPorts[i], targetLocations[i]);
			fputPaths(tempPort, startLocations[i], targetPorts[i], targetLocations[i], "False");
		}
	}
	
	
	
	
	fclose(TriggerConnections);
	fclose(pathSearchScript);
    if (line)
        free(line);
    exit(EXIT_SUCCESS);
}