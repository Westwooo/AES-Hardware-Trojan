#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./SBoxSearch.h"

struct connection connections[1000000];
int connectionIndex = 0;

int testing = 1;

char lutTiles[10][30];
char foundTile[20];
int foundIndex = 0;
int searchDone = 0;

void nextLUT(char tile[], char name[], int depth, int lutCount) {
		
	if(searchDone == 1)
		return;
		
	int newConnections = 1;
	char searchName[30];
	char searchTile[30];
	char nextSearchName[30];
	char nextSearchTile[30];
	char lutTest[4];
	struct connection * branchPoints[5][5];
	int branches = 0;
	
	//Initialise the branchPoints arrary
	for(int i=0; i<5; i++) {
		for(int j=0; j<5; j++){
			branchPoints[i][j] = NULL;
		}
	}
	
	strcpy(searchTile, tile); 
	strcpy(searchName, name);
	
	while(newConnections > 0) {
		
		newConnections = 0;
		
		for(int cIndex = 0; cIndex < connectionIndex; cIndex++) {
				
			if(strcmp(searchTile, connections[cIndex].beginTile) == 0 && strcmp(searchName, connections[cIndex].beginName) == 0) {
				
				//TESTING
				if(testing == 1) {
					printConnection(connections[cIndex]);
				}
				
				newConnections++;
				
				for(int i = 0; i < 3; i++) {
					lutTest[i] = connections[cIndex].endName[i + 2];
				}
				lutTest[3] = 0;
				//printf(lutTest);
				
				if(strcmp(lutTest, "LUT") == 0) {
					//printConnection(connections[cIndex]);
					
					if(lutCount > 0 && strcmp(foundTile, connections[cIndex].endTile) != 0) {
						
						printf("HERE\n");

						return;
						
						printConnection(connections[cIndex]);
						
						//TO DO - search the s-boxes and see if found LUT is part of an S-Box
						//If so set the next s-box pointer as appropriate
						//Else return 
						
					}
					else {
						strcpy(foundTile, connections[cIndex].endTile);
						lutCount++;
					}
					
					
				}
				
				if(newConnections == 1) {
					strcpy(nextSearchName, connections[cIndex].endName);
					strcpy(nextSearchTile, connections[cIndex].endTile);
				}
				else {
					//A branch has been enountered, so safe other routes accordingly
					branchPoints[branches][newConnections - 2] = &connections[cIndex];
				}
			}
				
		}//for
		
		if(newConnections > 1) {
			branches++;
		}
		
		//TESTING
		if(testing == 1) {
			printf("--------------------------------------------------------\n");
		}
		
		strcpy(searchName, nextSearchName);
		strcpy(searchTile, nextSearchTile);
		
	}//while
	
	if(depth > 0) {
		for(int i = branches - 1; i >= 0; i--) {
			for(int j = 0; j < 5; j++) {
				if(branchPoints[i][j] != NULL) {
					if(testing == 1) {
						printf("				-----BRANCH DEPTH %d-----\n", depth);
						printConnection(*branchPoints[i][j]);
						printf("--------------------------------------------------------\n");
					}
					nextLUT(branchPoints[i][j]->endTile, branchPoints[i][j]->endName, depth - 1, lutCount);
				}
			}
		}
	}
}

int main (int argc, char *argv[]) {
	
	//Open the json file to be processed and check success
	if(argc == 2) {
		char fileName[40] = "./";
		RouteData = fopen(strcat(fileName, argv[1]), "r");
		if(RouteData == NULL) {
			printf("Unable to open file\n");
			exit(EXIT_FAILURE);
		}
	}
	else {
		printf("Please supply a file path to be processed\n");
		exit(EXIT_FAILURE);
	}
	
	//Read all connections from JSON file and store in connections[]
	while(strcmp(word, "LUT_VALUES") != 0) {
		
		//Find the start of the next word
		getNextWord(word);
	
		//"attributes" indicates the begining of a connection
		if (strcmp(word, "attributes") == 0) {
		
			getNextWord(word);
		
			//Read and set the begining and end tiles and names
			connections[connectionIndex] = newConnection();
			setTileAndName(connections[connectionIndex].beginTile, connections[connectionIndex].beginName);
			setTileAndName(connections[connectionIndex].endTile, connections[connectionIndex].endName);	
			connectionIndex++;
		}
	}
	
	
	struct connection lutConnection;
	
	//TO DO - this will set the following S-Box in the full tool 
	//nextLUT("CLEL_L_X15Y111", "FFMUXC1_OUT1", 1, 0);
	nextLUT("CLEL_R_X6Y170", "FFMUXC1_OUT1", 1, 0);
	
}