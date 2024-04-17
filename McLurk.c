#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/ip.h>
#include<arpa/inet.h>
#include<stdio.h>
#include<unistd.h>
#include<signal.h>
#include<stdint.h>
#include<string.h>
#include<stdlib.h>
#include <glib.h>
#include <stdbool.h>
#include <time.h>
#include <endian.h>


#define MESSAGE_TYPE 1
#define CHANGEROOM_TYPE 2
#define FIGHT_TYPE 3
#define PVPFIGHT_TYPE 4
#define LOOT_TYPE 5
#define START_TYPE 6
#define ERROR_TYPE 7
#define ACCEPT_TYPE 8
#define ROOM_TYPE 9
#define CHARACTER_TYPE 10
#define GAME_TYPE 11
#define CONNECTION_TYPE 13
#define VERSION_TYPE 14

#define NARRATION_MARKER 1

#define TOTAL_ROOMS 10

#define MAJOR 2
#define MINOR 3

#define FLAG_ALIVE        0x80  // 1000 0000
#define FLAG_JOIN_BATTLE  0x40  // 0100 0000
#define FLAG_MONSTER      0x20  // 0010 0000
#define FLAG_STARTED      0x10  // 0001 0000
#define FLAG_READY        0x08  // 0000 1000
#define FLAG_MANAGER_KEY_FOUND 0x04 //0000 0100
#define FLAG_BASEMENT_KEY_FOUND 0x02 //0000 0010

#define ERROR_INVALID_STATS 1
#define ERROR_CHARACTER_NOT_FOUND 2
#define ERROR_INVALID_ROOM_NUMBER 3
#define ERROR_INVALID_ACTION 4
#define ERROR_RECIPIENT_NOT_FOUND 5
#define ERROR_NO_PVP 8

#define INITIAL_STATS 150
#define MIN_ROOM 1
#define MAX_ROOM 10

#define NOTIF_ROOM_LOCKED 1

typedef struct{
	uint8_t type;
	uint16_t length;
	char recipientName[32];
	char senderName[30];
	char message[];
}LurkMessage;

typedef struct {
    uint8_t type;
    uint16_t roomNumber;
    char roomName[32];
    uint16_t descriptionLength;
    char* description;
    uint16_t connections[10];
    int numOfConnections;
} LurkRoom;

typedef struct{
	char characterName[32];
	uint8_t flags;
	uint16_t attack;
	uint16_t defense;
	uint16_t regen;
	int16_t health;
	uint16_t gold;
	uint16_t currentRoom;
	uint16_t descriptionLength;
	char* playerDescription;
}Character;

typedef struct{
	uint8_t type;
	uint16_t initialPoints;
	uint16_t statLimit;
	uint16_t descriptionLength;
	char* description;
}GameStart;

typedef struct{
	uint8_t type;
	uint8_t major;
	uint8_t minor;
	uint16_t extLength;
	char* extensions;
}Version;

//rooms
/* 
   1 = mainCounter;
   2 = kitchen;
   3 = playPlace;
   4 = driveThru;
   5 = freezer;
   6 = storageRoom;
   7 = managerOffice;
   8 = mccafe;
   9 = basement;
   10 = rooftop; */

LurkRoom mainCounter ={ //connects to kitchen, mcCafe
	.roomNumber = 1,
	.roomName = "Main Counter",
	.description = "Beneath the dim glow of the menu screens, the main counter is shrouded in shadows and silence. The chill air mingles with the smell of old fries. The floor is sticky with spilled soda from the dinner rush. ",
	.connections = {2, 8},
	.numOfConnections = 2,
   
};

LurkRoom kitchen={//connects to main counter, freezer, storage room, managers office, drive thru
	.roomNumber = 2,
	.roomName = "Kitchen",
	.description = "A labyrinth of stainless steal, echoes of of culinary battles linger. Shadows move swifly across cold grills still covered in burger grease and bubbling fryers. ",
	.connections = {1, 4, 5, 6, 7},
	.numOfConnections = 5
};

LurkRoom playPlace = {
    .roomNumber = 3,
    .roomName = "Play Place",
    .description = "A colorful maze of slides and tunnels. Soft whispers of laughing children still linger throughout the room.",
    .connections = {8, 9}, 
    .numOfConnections = 2
};


LurkRoom driveThru = {//connects to play place, rooftop
	.roomNumber = 4,
	.roomName = "Drive Thru",
	.description = "An abandoned path that circles the building. The speaker hums waiting patiently for an order. The rustling of leaves sound like approaching vehicles. ",
	.connections = {2, 10},
	.numOfConnections = 2
};

LurkRoom freezer = {//connects to kitchen
	.roomNumber = 5,
	.roomName = "Freezer",
	.description = "A vault of ice, a frozen tomb. The chill air seeps to your bones. The flickering light casts shadows that move across the boxes. The only noise is the deep hum of the freezer. ",
	.connections = {2},
	.numOfConnections = 1
};

LurkRoom storageRoom={//connects to kitchen
	.roomNumber = 6,
	.roomName = "Storage Room",
	.description = "Cramped and cluttered, filled with cleaning supplies and items from past promotions. Your breaths disturb the layers of dust on the shelves that never get cleaned. ",
	.connections = {2},
	.numOfConnections = 1
};

LurkRoom managerOffice={//connects to kitchen, main counter, storage room
	.roomNumber = 7,
	.roomName = "Managers Office",
	.description = "A small confined room. Inspirational posters and past schedules line the walls. The desk is cluttered with papers, and a broken office chair that no longer raises or lowers. ",
	.connections= {1, 2, 6},
	.numOfConnections = 3
};

LurkRoom mccafe ={//connects to the main counter
	.roomNumber = 8,
	.roomName = "McCafe",
	.description = "A cozy, dimly lit nook where the scent of stale coffee lingers. Newspapers once read over a cup of coffee cover the table. ",
	.connections = {1, 3},
	.numOfConnections = 2
};

LurkRoom basement ={//connects play place
	.roomNumber = 9,
	.roomName = "Basement",
	.description = "The dark and damp foundation of your place of employment. ",
	.connections = {3},
	.numOfConnections = 1	
};

LurkRoom rooftop ={//connects to drive thru
	.roomNumber = 10,
	.roomName = "Rooftop ",
	.description = "A flat roof, the pipes are pumping out fumes. The roof gives you a panoramic view of the city. ",
	.connections = {4},
	.numOfConnections = 1
};

LurkRoom allRooms[TOTAL_ROOMS]; 

void initializeAllRooms() {
    allRooms[0] = mainCounter;
    allRooms[1] = kitchen;
    allRooms[2] = playPlace;
    allRooms[3] = driveThru;
    allRooms[4] = freezer;
    allRooms[5] = storageRoom;
    allRooms[6] = managerOffice;
    allRooms[7] = mccafe;
    allRooms[8] = basement;
    allRooms[9] = rooftop;
}

GHashTable* characterTable = NULL;
GHashTable* clientsTable = NULL;
pthread_mutex_t characterTableMutex = PTHREAD_MUTEX_INITIALIZER;


typedef struct {
    char name[32];
    int client_fd;
} ClientKey;

ClientKey* createClientKey(const char* name, int client_fd) {
    ClientKey* key = malloc(sizeof(ClientKey));
    if (key != NULL) {
        strncpy(key->name, name, 31);
        key->name[31] = '\0'; 
        key->client_fd = client_fd;
    }
    return key;
}

guint client_key_hash(gconstpointer key) {
    const ClientKey* clientKey = key;
    return g_str_hash(clientKey->name) ^ g_direct_hash(GINT_TO_POINTER(clientKey->client_fd));
}

gboolean client_key_equal(gconstpointer a, gconstpointer b) {
    const ClientKey* keyA = a;
    const ClientKey* keyB = b;
    return g_str_equal(keyA->name, keyB->name) && keyA->client_fd == keyB->client_fd;
}

void addClient(int client_fd, const char* name) {
    if (clientsTable != NULL && name != NULL) {
        ClientKey* key = createClientKey(name, client_fd);
        g_hash_table_insert(clientsTable, key, GINT_TO_POINTER(client_fd)); // Key ownership is transferred to the table
    }
}

void sendErrorMessage(int client_fd, uint8_t errorCode, const char* errorMsg) {
    uint8_t messageType = ERROR_TYPE; 
    uint16_t messageLength = (strlen(errorMsg)); 
    uint8_t messageCode = errorCode; 

    size_t totalMessageSize = 1 + 1 + 2 + strlen(errorMsg);

    char* messageBuffer = malloc(totalMessageSize);
    if (!messageBuffer) {
        perror("Allocation failure for error message");
        exit(EXIT_FAILURE);
    }

    size_t offset = 0;
    messageBuffer[offset++] = messageType;
    messageBuffer[offset++] = messageCode;
    memcpy(messageBuffer + offset, &messageLength, sizeof(messageLength));
    offset += sizeof(messageLength);
    memcpy(messageBuffer + offset, errorMsg, strlen(errorMsg));

    ssize_t bytesSent = send(client_fd, messageBuffer, totalMessageSize, 0);
    if (bytesSent < 0) {
        perror("Failed to send error message");
    } else if ((size_t)bytesSent < totalMessageSize) {
        fprintf(stderr, "Partial send. Only %zd bytes out of %zu were sent for error message.\n", bytesSent, totalMessageSize);
    }

    free(messageBuffer);
}


uint16_t toLittleEndian16(uint16_t value) {
    return htole16(value);
}

void sendAcceptMessage(int clientSocket, uint8_t acceptedMessageType) {
    uint8_t messageType = ACCEPT_TYPE; 
    uint8_t message[2] = {messageType, acceptedMessageType};

    ssize_t bytesSent = send(clientSocket, message, sizeof(message), 0);
    if (bytesSent < 0) {
        perror("Failed to send accept message");
    } else if ((size_t)bytesSent < sizeof(message)) {
        fprintf(stderr, "Partial send. Only %zd bytes sent out of %zu expected.\n", bytesSent, sizeof(message));
    } else {
        printf("Accept message sent for message type %d.\n", acceptedMessageType);
    }
}

void sendWelcomeMessage(int clientSocket, const char* playerName) {
    uint8_t messageType = MESSAGE_TYPE; 
    char welcomeText[256]; 
    snprintf(welcomeText, sizeof(welcomeText), "Welcome to the game, %s!", playerName);

    uint16_t messageLength = htons(strlen(welcomeText)); 

    size_t totalMessageSize = 1 + sizeof(messageLength) + strlen(welcomeText);

    char* messageBuffer = malloc(totalMessageSize);
    if (!messageBuffer) {
        perror("Allocation failure for welcome message");
        exit(EXIT_FAILURE);
    }

    size_t offset = 0;
    messageBuffer[offset++] = messageType;
    memcpy(messageBuffer + offset, &messageLength, sizeof(messageLength));
    offset += sizeof(messageLength);
    memcpy(messageBuffer + offset, welcomeText, strlen(welcomeText));

    ssize_t bytesSent = send(clientSocket, messageBuffer, totalMessageSize, 0);
    if (bytesSent < 0) {
        perror("Failed to send welcome message");
    } else if ((size_t)bytesSent < totalMessageSize) {
        fprintf(stderr, "Partial send. Only %zd bytes out of %zu were sent for welcome message.\n", bytesSent, totalMessageSize);
    } else {
        printf("Welcome message sent successfully to %s.\n", playerName);
    }

    free(messageBuffer);
}


void updateCharacterStatus(int client_fd, const Character* newStatus) {
    pthread_mutex_lock(&characterTableMutex);
    
    Character* characterToUpdate = g_hash_table_lookup(characterTable, &client_fd);
    if (characterToUpdate == NULL) {
        // Handle character not found
        pthread_mutex_unlock(&characterTableMutex);
        return;
    }
    

    characterToUpdate->currentRoom = newStatus->currentRoom;
    

    if (characterToUpdate->playerDescription != NULL) {
        free(characterToUpdate->playerDescription); 
    }
    characterToUpdate->playerDescription = strdup(newStatus->playerDescription); 
    

    pthread_mutex_unlock(&characterTableMutex);

}




void sendRoomDescription(int client_fd, const LurkRoom room) {
  
    uint16_t descriptionLength = strlen(room.description);  
  
    size_t messageSize = 1 + // Type
                         2 + // Room Number (little-endian)
                         32 + // Room Name
                         2 + // Description Length (little-endian)
                         descriptionLength; // Description

    uint8_t* message = malloc(messageSize);
    if (!message) {
        perror("Failed to allocate memory for room description message");
        return;
    }

    size_t offset = 0;

   
    message[offset++] = ROOM_TYPE; 

    *((uint16_t*)(message + offset)) = room.roomNumber; 
    offset += 2;


    memcpy(message + offset, room.roomName, 32);
    offset += 32;

    *((uint16_t*)(message + offset)) = descriptionLength; 
    offset += 2;


    memcpy(message + offset, room.description, descriptionLength);

    if (send(client_fd, message, messageSize, 0) < 0) {
        perror("Failed to send room description");
    } else {
        printf("Sent ROOM message:\nType: %d\nRoom Number: %d\nDescription Length: %d\n",
               ROOM_TYPE, room.roomNumber, descriptionLength);
        printf("\n");
    }

    free(message);
}

//void sendRoomDescription(int clientSocket, LurkRoom room);

LurkRoom getConnectedRoom(uint16_t roomNumber) {
    for(int i = 0; i < TOTAL_ROOMS; i++) {
        if(allRooms[i].roomNumber == roomNumber) {
            return allRooms[i];
        }
    }

    // If room is not found, return an empty room structure
    LurkRoom emptyRoom = {0};
    return emptyRoom;
}


void sendRoomConnections(int clientSocket, const LurkRoom currentRoom) {
    printf("Sending connections for room: %s\n", currentRoom.roomName);

    for (int i = 0; i < currentRoom.numOfConnections; ++i) {
        uint16_t connectedRoomNum = currentRoom.connections[i];
        // Find the connected room from allRooms array
        LurkRoom connectedRoom = getConnectedRoom(connectedRoomNum); 

        uint16_t roomNumberLE = htole16(connectedRoom.roomNumber);
        uint16_t descriptionLengthLE = htole16(strlen(connectedRoom.description));

        uint8_t messageType = 13; 

        size_t messageSize = 1 + sizeof(roomNumberLE) + 32 + sizeof(descriptionLengthLE) + strlen(connectedRoom.description);

        char *messageBuffer = (char *)malloc(messageSize);
        if (!messageBuffer) {
            perror("Allocation failure for connection message");
            return;
        }

        size_t offset = 0;
        messageBuffer[offset++] = messageType;

        memcpy(messageBuffer + offset, &roomNumberLE, sizeof(roomNumberLE));
        offset += sizeof(roomNumberLE);

        memset(messageBuffer + offset, 0, 32);
        strncpy(messageBuffer + offset, connectedRoom.roomName, 31);
        offset += 32;

        memcpy(messageBuffer + offset, &descriptionLengthLE, sizeof(descriptionLengthLE));
        offset += sizeof(descriptionLengthLE);

        memcpy(messageBuffer + offset, connectedRoom.description, strlen(connectedRoom.description));
        
        printf("Sending connection to room: %s, Number: %d\n", connectedRoom.roomName, connectedRoom.roomNumber);

        if (send(clientSocket, messageBuffer, messageSize, 0) < 0) {
            perror("Failed to send connection message");
        }

        free(messageBuffer);
    }
}

LurkRoom getCurrentRoom(uint16_t roomNumber) {
    for (int i = 0; i < TOTAL_ROOMS; i++) {
        if (allRooms[i].roomNumber == roomNumber) {
            return allRooms[i];
        }
    }
    // Return an empty room if not found
    return (LurkRoom){0};
}

void sendPrompt(int client_fd, uint8_t type, const char* prompt) {
    uint16_t len = htons(strlen(prompt)); 
    send(client_fd, &type, sizeof(type), 0);
    send(client_fd, &len, sizeof(len), 0);
    send(client_fd, prompt, strlen(prompt), 0);
}

void readResponse(int client_fd, char* buf, size_t buf_size) {
    uint16_t res_length;
    recv(client_fd, &res_length, sizeof(res_length), 0);
    res_length = ntohs(res_length); 
    recv(client_fd, buf, res_length, 0);
    buf[res_length] = '\0'; 
}


void initializeCharacterStorage() {
    characterTable = g_hash_table_new_full(g_int_hash, g_int_equal, free, free);
}

void storeCharacter(int client_fd, Character* character) {
    pthread_mutex_lock(&characterTableMutex);
    
    int* key = malloc(sizeof(int));
    if (!key) {
        fprintf(stderr, "Failed to allocate memory for key.\n");
        pthread_mutex_unlock(&characterTableMutex);
        return;
    }
    *key = client_fd;

    Character* storedCharacter = malloc(sizeof(Character));
    if (!storedCharacter) {
        fprintf(stderr, "Failed to allocate memory for stored character.\n");
        free(key);  
        pthread_mutex_unlock(&characterTableMutex);
        return;
    }
    memcpy(storedCharacter, character, sizeof(Character));

    storedCharacter->playerDescription = strdup(character->playerDescription);
    if (character->playerDescription && !storedCharacter->playerDescription) {
        fprintf(stderr, "Failed to duplicate character description.\n");
        free(key);
        free(storedCharacter);
        pthread_mutex_unlock(&characterTableMutex);
        return;
    }

    g_hash_table_replace(characterTable, key, storedCharacter);

    pthread_mutex_unlock(&characterTableMutex);
}

Character* retrieveCharacter(int client_fd) {
    printf("Inside retrieve Character before lock");
    //pthread_mutex_lock(&characterTableMutex);
    
    Character* character = g_hash_table_lookup(characterTable, &client_fd);
    
    pthread_mutex_unlock(&characterTableMutex);
    return character;
}

void receiveCharacter(int client_fd) {
    Character character;
    memset(&character, 0, sizeof(Character)); // Initialize character structure

    bool validPlayer = false;

    while (!validPlayer) {
        
        ssize_t bytesRead = read(client_fd, &character.characterName, 32);
        if (bytesRead <= 0) {
            sendErrorMessage(client_fd, ERROR_INVALID_STATS, "Failed to read character name.");
            return;
        }
        character.characterName[31] = '\0';

        uint8_t flags;
        bytesRead = read(client_fd, &flags, sizeof(flags));

        if (bytesRead <= 0) {
            sendErrorMessage(client_fd, ERROR_INVALID_STATS, "Failed to read character flags.");
            return;
        }
       
        
        character.flags = flags;
        if (character.flags & FLAG_MONSTER) {
            printf("Before removing monster flag: 0x%02x. \n ", character.flags);
            character.flags &= ~FLAG_MONSTER;
            printf("Adjusted flags after removing monster flag: 0x%02x. \n", character.flags);

        }

        uint16_t temp;
        int totalUsedStats = 0;
        bytesRead = read(client_fd, &temp, sizeof(temp)); 
        character.attack = (temp);
        printf("Received Attack: %u\n", character.attack);
        totalUsedStats += character.attack;

        bytesRead = read(client_fd, &temp, sizeof(temp)); 
        character.defense = (temp);
        printf("Received Defense: %u\n", character.defense);
        totalUsedStats += character.defense;

        bytesRead = read(client_fd, &temp, sizeof(temp)); 
        character.regen = (temp);
        printf("Received Regen: %u\n", character.regen);
    

        bytesRead = read(client_fd, &temp, sizeof(temp));
        character.health = (temp);
        printf("Received Health: %d\n", character.health);
        if (character.health < 0) {
            sendErrorMessage(client_fd, ERROR_INVALID_STATS, "Health must not be negative.");
            return;
        }

        int calculatedHealth = 50 + (character.defense + character.regen) / 2.5;

        character.health = calculatedHealth;

        bytesRead = read(client_fd, &temp, sizeof(temp)); 
        character.gold = (temp);
        printf("Received Gold: %u\n", character.gold);
        character.gold = 0;

        bytesRead = read(client_fd, &temp, sizeof(temp)); 
        character.currentRoom = (temp);
        printf("Received Current Room: %u\n", character.currentRoom);

        bytesRead = read(client_fd, &temp, sizeof(temp)); 
        character.descriptionLength = (temp);
        printf("Received Description Length: %u\n", character.descriptionLength);

         character.currentRoom = 1;
        if ((character.attack + character.defense + character.regen) > INITIAL_STATS) {
            sendErrorMessage(client_fd, ERROR_INVALID_STATS, "Total stats exceed the maximum allowed value.");
            return;
           
        }

     

        if (character.descriptionLength > 0) {
            character.playerDescription = (char*)malloc(character.descriptionLength + 1); 
            if (!character.playerDescription) {
                perror("Failed to allocate memory for character description");
                sendErrorMessage(client_fd, ERROR_INVALID_STATS, "Server error: memory allocation failed.");
                return;
            }

            bytesRead = read(client_fd, character.playerDescription, character.descriptionLength);

            if (bytesRead != character.descriptionLength) {
                free(character.playerDescription);
                sendErrorMessage(client_fd, ERROR_INVALID_STATS, "Failed to read full character description.");
                return;
            }
            character.playerDescription[character.descriptionLength] = '\0'; 
            character.descriptionLength = bytesRead;

        }

          validPlayer = true;
            storeCharacter(client_fd, &character);
            addClient(client_fd, character.characterName);

            sendAcceptMessage(client_fd, CHARACTER_TYPE);
    }


    
    //sendWelcomeMessage(client_fd, character.characterName);

    printf("Character '%s' received and accepted.\n", character.characterName);
    printf("\n");
    printf("Character Received:\nName: %s\nFlags: 0x%02x\n", character.characterName, character.flags);
    if (character.flags & FLAG_ALIVE) printf(" - Alive\n");
    if (character.flags & FLAG_JOIN_BATTLE) printf(" - Join Battle\n");
    if (character.flags & FLAG_MONSTER) printf(" - Monster\n");
    if (character.flags & FLAG_STARTED) printf(" - Started\n");
    if (character.flags & FLAG_READY) printf(" - Ready\n");

    printf("Attack: %u\nDefense: %u\nRegen: %u\nHealth: %d\nGold: %u\nCurrent Room: %u\nDescription Length: %u\nDescription: %s\n",
           character.attack, character.defense, character.regen, character.health, character.gold, character.currentRoom, character.descriptionLength, character.playerDescription);
}



void sendCharacterMessage(int clientSocket, Character character) {
    size_t messageSize = 1 + 32 // Name
                         + 1  // Flags
                         + 2  // Attack
                         + 2  // Defense
                         + 2  // Regen
                         + 2  // Health
                         + 2  // Gold
                         + 2  // Current room
                         + 2  // Description length
                         + character.descriptionLength; // Description

    char* message = (char*)malloc(messageSize);
    if (!message) {
        perror("Allocation failed for character message");
        return;
    }

    size_t offset = 0;
    message[offset++] = CHARACTER_TYPE;

    memcpy(message + offset, character.characterName, 32);
    offset += 32;

    message[offset++] = character.flags;

    *((uint16_t*)(message + offset)) = character.attack; 
    offset += 2;
    *((uint16_t*)(message + offset)) = character.defense;
    offset += 2;
    *((uint16_t*)(message + offset)) = character.regen;
    offset += 2;
    *((int16_t*)(message + offset)) = character.health;
    offset += 2;
    *((uint16_t*)(message + offset)) = character.gold;
    offset += 2;
    *((uint16_t*)(message + offset)) = character.currentRoom;
    offset += 2;

  
    *((uint16_t*)(message + offset)) = character.descriptionLength; 
    offset += 2;
    if (character.descriptionLength > 0 && character.playerDescription != NULL) {
        memcpy(message + offset, character.playerDescription, character.descriptionLength);
    }

    ssize_t bytesSent = send(clientSocket, message, messageSize, 0);
    if (bytesSent < 0) {
        perror("Failed to send character message");
    } else if ((size_t)bytesSent < messageSize) {
        fprintf(stderr, "Partial send. Only %zd bytes out of %zu were sent.\n", bytesSent, messageSize);
    }

 
    free(message);
}




void gameStart(int clientSocket) {
const char startDescription[] = 
"\n\n"
" ____    ____         _____                    __      _        \n"
"|_   \\  /   _|       |_   _|                  [  |  _ | |       \n"
"  |   \\/   |   .---.   | |     __   _   _ .--. | | / ]\\_|.--.   \n"
"  | |\\  /| |  / /'`\\]  | |   _[  | | | [ `/'`\\]| '' <   ( (`\\]  \n"
" _| |_\\/_| |_ | \\__.  _| |__/ || \\_/ |, | |    | |`\\ \\   `'.'.  \n"
"|_____||_____|'.___.'|________|'.__.'_/[___]  [__|  \\_] [\\__) ) \n"
"\n"
"You find yourself at the entrance of McLurk's, a once thriving fast food joint now whispered about in hushed tones. The once shining golden arches now flicker erratically. Rumors speak of the secret sauce once used on a promotional item is lost within the joint. Your adventure begins here in McLurk's. Will you uncover the truth or become a lost soul on the quest for sauce?";    uint16_t descriptionLength = strlen(startDescription); // Already in little-endian

    uint16_t initialPoints = INITIAL_STATS; 
    uint16_t statLimit = 65535;   

    size_t messageSize = 1 + sizeof(initialPoints) + sizeof(statLimit) + sizeof(descriptionLength) + descriptionLength;
    char* message = malloc(messageSize);
    if (!message) {
        perror("Allocation failure");
        exit(EXIT_FAILURE);
    }

    size_t offset = 0;
    message[offset++] = GAME_TYPE; 
    memcpy(message + offset, &initialPoints, sizeof(initialPoints)); 
    offset += sizeof(initialPoints);
    memcpy(message + offset, &statLimit, sizeof(statLimit)); 
    offset += sizeof(statLimit);
    memcpy(message + offset, &descriptionLength, sizeof(descriptionLength)); 
    offset += sizeof(descriptionLength);
    memcpy(message + offset, startDescription, descriptionLength); 
    
    ssize_t bytesSent = send(clientSocket, message, messageSize, 0);
    if (bytesSent < 0) {
        perror("Send failed");
    } else if ((size_t)bytesSent < messageSize) {
        fprintf(stderr, "Partial send. Only %zd bytes sent out of %zu expected.\n", bytesSent, messageSize);
    }

    // Clean up
    free(message);
}

void listCharactersInRoom(int client_fd, uint16_t roomNumber) {
    GHashTableIter iter;
    gpointer key, value;
    Character *character;

    printf("Listing characters in room: %u\n", roomNumber); 
    g_hash_table_iter_init(&iter, characterTable);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        character = (Character *)value;
        if (character->currentRoom == roomNumber) {
            sendCharacterMessage(client_fd, *character);

            printf("Retrieved character: %s\n", character->characterName);
            printf(" - Room: %u\n", character->currentRoom); 
            printf(" - Attack: %u, Defense: %u, Regen: %u, Health: %d, Gold: %u\n",
                   character->attack, character->defense, character->regen, character->health, character->gold); 
            printf(" - Description: %s\n\n", character->playerDescription); 
        }
    }
}

GList* gatherCharactersInRoom(uint16_t roomNumber) {
    GHashTableIter iter;
    gpointer key, value;
    GList* charactersInRoom = NULL;  // Create an empty list to hold characters

    g_hash_table_iter_init(&iter, characterTable);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        Character* character = (Character*)value;
        if (character->currentRoom == roomNumber) {
            charactersInRoom = g_list_append(charactersInRoom, character);
        }
    }
    return charactersInRoom;
}


void sendMessage(int client_fd, const char* recipientName, const char* senderName, const char* messageContent, int isNarration) {
    uint8_t messageType = MESSAGE_TYPE;
    uint16_t messageLength = strlen(messageContent);
    uint16_t netMessageLength = toLittleEndian16(messageLength); 
    
    // Prepare packet
    size_t packetSize = 1 + 2 + 32 + 32 + messageLength; 
    char* packet = (char*)malloc(packetSize);
    if (packet == NULL) {
        perror("Failed to allocate packet");
        return;
    }

    size_t offset = 0;
    packet[offset++] = messageType;

    memcpy(packet + offset, &netMessageLength, sizeof(netMessageLength));
    offset += sizeof(netMessageLength);

    memset(packet + offset, 0, 32);
    strncpy(packet + offset, recipientName, 31); 
    offset += 32;

    memset(packet + offset, 0, 32);
    strncpy(packet + offset, senderName, 31); 
    offset += 32;

    memcpy(packet + offset, messageContent, messageLength);
    
    ssize_t bytesSent = send(client_fd, packet, packetSize, 0);
    if (bytesSent < 0) {
        perror("send failed");
    }

    free(packet);
}

int max(int a, int b) {
    if (a > b) {
        return a;
    } else {
        return b;
    }
}


void changeRoom(int client_fd, uint16_t newRoomNumber) {
    pthread_mutex_lock(&characterTableMutex);

    Character* character = g_hash_table_lookup(characterTable, &client_fd);
    if (character == NULL) {
        sendErrorMessage(client_fd, ERROR_CHARACTER_NOT_FOUND, "Character not found.");
        pthread_mutex_unlock(&characterTableMutex);
        return;
    }

    LurkRoom currentRoom = getCurrentRoom(character->currentRoom);
    bool isValidConnection = false;
    for (int i = 0; i < currentRoom.numOfConnections; ++i) {
        if (currentRoom.connections[i] == newRoomNumber) {
            isValidConnection = true;
            break;
        }
    }

    if (!isValidConnection) {
        sendErrorMessage(client_fd, ERROR_INVALID_ROOM_NUMBER, "Invalid room number.");
        pthread_mutex_unlock(&characterTableMutex);
        return; 
    }
    if (newRoomNumber == 7 && !(character->flags & FLAG_MANAGER_KEY_FOUND)) {
        sendMessage(client_fd, character->characterName, "Narrator", "The manager office door is locked. Try a different room. ", 1);
        pthread_mutex_unlock(&characterTableMutex);
        return; 
    }
    if (newRoomNumber == 9 && !(character->flags & FLAG_BASEMENT_KEY_FOUND)) {
        sendMessage(client_fd, character->characterName, "Narrator", "The basement door is locked. Try a different room. ", 1);
        pthread_mutex_unlock(&characterTableMutex);
        return; 
    }

    if (newRoomNumber == 10 && !(character->flags & FLAG_MANAGER_KEY_FOUND)) {
        sendMessage(client_fd, character->characterName, "Narrator", "You found the office key", 1);
        character->flags |= FLAG_MANAGER_KEY_FOUND;

    }
     if (newRoomNumber == 7 && !(character->flags & FLAG_BASEMENT_KEY_FOUND)) {
        sendMessage(client_fd, character->characterName, "Narrator", "You found the basement key", 1);
        character->flags |= FLAG_BASEMENT_KEY_FOUND;

    }



    character->currentRoom = newRoomNumber;

    LurkRoom newRoom = getCurrentRoom(newRoomNumber);

    sendRoomDescription(client_fd, newRoom);

    listCharactersInRoom(client_fd, newRoomNumber);
    sendRoomConnections(client_fd, newRoom);


    pthread_mutex_unlock(&characterTableMutex);
}

void createMonster(int monster_id, const char* name, uint16_t attack, uint16_t defense, uint16_t regen, int16_t health, uint16_t gold, uint16_t room, const char* description) {
    Character* monster = malloc(sizeof(Character));
    if (!monster) {
        perror("Allocation failure for monster");
        return;
    }

    memset(monster, 0, sizeof(Character));
    strncpy(monster->characterName, name, 31);
    monster->flags = FLAG_MONSTER | FLAG_ALIVE; 
    monster->attack = attack;
    monster->defense = defense;
    monster->regen = regen;
    monster->health = health;
    monster->gold = gold;
    monster->currentRoom = room;
    monster->descriptionLength = strlen(description);
    monster->playerDescription = strdup(description);

    int* key = malloc(sizeof(int));
    *key = monster_id;

    pthread_mutex_lock(&characterTableMutex);
    g_hash_table_replace(characterTable, key, monster);
    pthread_mutex_unlock(&characterTableMutex);

    printf("Monster '%s' created and placed in room %d.\n", name, room);
}

void initializeMonsters() {
    // id, name, attack, defense, regen, health, gold, room, description
    createMonster(1, "Order Bot", 30, 60, 60, 98, 20, 1, "A sleek, metallic humanoid figure with glowing red eyes and sharp, mechanical limbs, its body adorned with buttons and screens.");
    createMonster(2, "Burger Fiend", 50, 50, 50, 90, 25, 2, "A towering, grotesque amalgamation of meat patties and burger toppings, its form constantly shifting and oozing with greasy juices.");
    createMonster(3, "Fry Fiend", 40, 55, 55, 94, 25, 2, "A writhing mass of sentient french fries, their golden exteriors glistening with oil as they writhe and squirm in unsettling patterns.");
    //for some reason there needs to be two monsters in room 3 for one to appear
    createMonster(4, "BallPit Beast", 45, 50, 55, 92, 25, 3, "A monstrous entity lurking beneath the colorful plastic spheres of the ball pit, its form obscured by shadows and shifting shapes.");
    createMonster(12, "BallPit Beast", 45, 50, 55, 92, 25, 3, "A monstrous entity lurking beneath the colorful plastic spheres of the ball pit, its form obscured by shadows and shifting shapes.");
    createMonster(5, "Drive Thru Demon", 40, 55, 55, 94, 35, 4, "A towering figure wreathed in smoke and shadow, its eyes burning with malevolent intent as it surveys its domain from the depths of the drive-thru lane.");
    createMonster(6, "Frozen Phantom", 35, 57, 58, 96, 35, 5, "A spectral figure draped in tattered freezer-burned garments, its icy breath frosting the air with each haunting exhale.");
    //createMonster(13, "Frozen Phantom", 35, 57, 58, 96, 35, 5, "A spectral figure draped in tattered freezer-burned garments, its icy breath frosting the air with each haunting exhale.");
    createMonster(7, "Corporate Crony", 40, 55, 55, 94, 60, 7, "A sharp-suited figure with a plastic smile permanently etched onto its face, its eyes empty voids of soulless ambition.");
    createMonster(8, "Coffee Imp", 45, 50, 55, 92, 15, 8, "A mischievous sprite made of swirling ice and coffee, darting around the McCafe and causing chaos wherever it goes.");
    createMonster(9, "Cappuccino Creeper", 40, 55, 55, 94, 20, 8, "A stealthy predator that blends into the frothy foam of cappuccinos, ambushing its prey with silent efficiency.");
    createMonster(10, "Ronald McRotten", 50, 50, 50, 90, 50, 9, "Ronald McRotten, once the epitome of joy in the realm of fast food, now embodies a grotesque perversion of his former self. His vibrant red hair now hangs matted with filth, his yellow jumpsuit torn and stained with corruption. Within his empty eyes flickers the flame of McMadness, driving him to torment all who cross his path. His echoing laughter instills fear in the bravest. Clutched tightly in his hand is the legendary sauce, its eerie glow illuminating his dark domain.");
    createMonster(11, "Gargoyle Guardians", 45, 50, 55, 92, 45, 10, "A towering figure wreathed in smoke and shadow, its eyes burning with malevolent intent as it surveys its domain from the depths of the drive-thru lane.");
}

void version(int clientSocket){
	uint8_t message[5];
	message[0] = VERSION_TYPE;
	message[1] = MAJOR;
	message[2] = MINOR;
	uint16_t extLength = htons(0);

	memcpy(&message[3], &extLength, sizeof(extLength));

	ssize_t bytesSent = send(clientSocket, message, sizeof(message), 0);
	//bytesSent += send(clientSocket, &extLength, sizeof(extLength), 0);



	if(bytesSent < 0){
		perror("Failed to send");
	}else if(bytesSent < sizeof(message)){
		fprintf(stderr, "Partial send, Only %zd of version was send!\n", bytesSent);
	}

}


void printRawBytes(unsigned char* buffer, ssize_t length) {
    printf("Raw bytes received: ");
    for (ssize_t i = 0; i < length; ++i) {
        printf("%02x ", buffer[i]);
    }
    printf("\n");
}
void printClient(gpointer key, gpointer value, gpointer user_data) {
    printf("Client name: %s, fd: %d\n", (char*)key, GPOINTER_TO_INT(value));
}

int findClientFdByName(const char* name) {
    if (!clientsTable) {
        printf("clientsTable has not been initialized.\n");
        return -1; 
    }

    printf("Listing all clients in the table:\n");
    g_hash_table_foreach(clientsTable, (GHFunc)printClient, NULL);

    ClientKey tempKey;
    strncpy(tempKey.name, name, 31);
    tempKey.name[31] = '\0'; 

    gpointer client_fd_ptr = NULL;
    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init(&iter, clientsTable);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        ClientKey* currentKey = (ClientKey*)key;
        if (g_str_equal(currentKey->name, tempKey.name)) {
            client_fd_ptr = value;
            break;
        }
    }

    if (client_fd_ptr) {
        printf("Found client_fd for name '%s': %d\n", name, GPOINTER_TO_INT(client_fd_ptr));
        return GPOINTER_TO_INT(client_fd_ptr);
    } else {
        printf("No entry found for name '%s'.\n", name);
        return -1;
    }
}



void fight(int client_fd) {
    sendAcceptMessage(client_fd, FIGHT_TYPE);
    printf("Initiating fight for client %d\n", client_fd);

    srand(time(NULL)); 

    Character* player = retrieveCharacter(client_fd);
/*     if (!player || !(player->flags & FLAG_ALIVE) || !(player->flags & FLAG_JOIN_BATTLE)) {
        sendErrorMessage(client_fd, ERROR_CHARACTER_NOT_FOUND, "You cannot fight right now.");
        return;
    } */

    bool allMonstersDefeated = false;

    while (player->health > 0 && !allMonstersDefeated) {
        int playerInitialHealth = player->health;

        printf("Player [%s] stats: Attack=%d, Defense=%d, Regen=%d, Health=%d\n", 
            player->characterName, player->attack, player->defense, player->regen, player->health);

        GList* monstersInRoom = gatherCharactersInRoom(player->currentRoom);
        if (!monstersInRoom) {
            sendErrorMessage(client_fd, ERROR_INVALID_ROOM_NUMBER, "No enemies to fight here.");
            return;
        }

        for (GList* node = monstersInRoom; node != NULL; node = g_list_next(node)) {
            Character* monster = (Character*)node->data;
            int monsterInitialHealth = monster->health;
            allMonstersDefeated = true;
            if ((monster->flags & FLAG_MONSTER) && (monster->flags & FLAG_ALIVE)) {
                allMonstersDefeated = false;
                printf("Engaging monster [%s] with Attack=%d, Defense=%d, Health=%d\n", 
                    monster->characterName, monster->attack, monster->defense, monster->health);

                int playerAttackRoll = (rand() % 10) + 1;
                printf("Player attack roll: %d\n", playerAttackRoll);

                int playerTotalAttack = playerAttackRoll + (player->attack / 10);
                printf("Player total attack (roll + attack/10): %d\n", playerTotalAttack);

                int monsterEffectiveDefense = monster->defense / 10;
                printf("Monster's effective defense (defense/10): %d\n", monsterEffectiveDefense);

                int damageDieRoll;
                char message[256];

                printf("Before attack, %s's flags: 0x%02x\n", monster->characterName, monster->flags);
                if (monster->flags & FLAG_ALIVE) printf("%s is alive\n", monster->characterName);
                if (monster->flags & FLAG_MONSTER) printf("%s is a monster\n", monster->characterName);


                // Player's attack logic
                if (playerTotalAttack > monsterEffectiveDefense) {
                    int baseDamageCalc = playerTotalAttack - monsterEffectiveDefense;
                    if (baseDamageCalc > 0) {
                        damageDieRoll = (rand() % 2) + baseDamageCalc;
                    } else {
                        damageDieRoll = (rand() % 2);
                    }
                    int damage = max(1, baseDamageCalc) + damageDieRoll;
                    printf("%d (Player Total Attack) - %d (Monster Defense) = %d, Damage Die Roll: %d, Total Damage: %d\n",
                        playerTotalAttack, monsterEffectiveDefense, baseDamageCalc, damageDieRoll, damage);
                    monster->health -= damage;
                    sprintf(message, "You hit %s for %d damage.", monster->characterName, damage);
                } else {
                    sprintf(message, "You missed %s.", monster->characterName);
                    printf("Player missed the attack on %s.\n", monster->characterName);
                }
                sendMessage(client_fd, player->characterName, "Narrator", message, 1);

                // Monster's attack logic
                if (monster->health > 0) {
                    int monsterAttackRoll = (rand() % 10) + 1;
                    printf("Monster attack roll: %d\n", monsterAttackRoll);

                    int monsterTotalAttack = monsterAttackRoll + (monster->attack / 10);
                    printf("Monster total attack (roll + attack/10): %d\n", monsterTotalAttack);

                    int playerEffectiveDefense = player->defense / 10;
                    printf("Player's effective defense (defense/10): %d\n", playerEffectiveDefense);

                    // Monster's attack damage calculation
                    if (monsterTotalAttack > playerEffectiveDefense) {
                        int baseDamageCalc = monsterTotalAttack - playerEffectiveDefense;
                        if (baseDamageCalc > 0) {
                            damageDieRoll = (rand() % 2) + baseDamageCalc;
                        } else {
                            damageDieRoll = (rand() % 2);
                        }
                        int damage = max(1, baseDamageCalc) + damageDieRoll;
                        printf("%d (Monster Total Attack) - %d (Player Defense) = %d, Damage Die Roll: %d, Total Damage: %d\n",
                            monsterTotalAttack, playerEffectiveDefense, baseDamageCalc, damageDieRoll, damage);
                        player->health -= damage;
                        sprintf(message, "%s hits you for %d damage.", monster->characterName, damage);
                    } else {
                        sprintf(message, "%s missed you.", monster->characterName);
                        printf("Monster missed the attack on player.\n");
                    }
                    sendMessage(client_fd, player->characterName, "Narrator", message, 1);
                }

                if (monster->health <= 0) {
                    sprintf(message, "You have defeated %s!", monster->characterName);
                    sendMessage(client_fd, "Narrator", player->characterName, message, 1);
                    
                    printf("Monster [%s] defeated by player.\n", monster->characterName);
                    printf("Before updating flags, %s's flags: 0x%02x\n", monster->characterName, monster->flags);
                    monster->flags &= ~FLAG_ALIVE;
                    printf("After updating flags, %s's flags: 0x%02x\n", monster->characterName, monster->flags);

                    monster->flags &= ~FLAG_ALIVE;
                

                }
                if (player->health > 0) { 
                    int playerRegenAmount = (player->regen / 10) * ((rand() % 5) + 1); // Calculate regeneration amount
                    player->health += playerRegenAmount; // Apply regeneration
                    if (player->health > playerInitialHealth) { 
                        player->health = playerInitialHealth;
                    }
                    printf("Player [%s] regenerates %d health, total health now %d.\n", player->characterName, playerRegenAmount, player->health);
                }

                // Monster regeneration logic
                if (monster->health > 0 && monster->health < monsterInitialHealth) { 
                    int monsterRegenAmount = (monster->regen / 10) * ((rand() % 2) + 1);
                    monster->health += monsterRegenAmount;
                    if (monster->health > monsterInitialHealth) {
                        monster->health = monsterInitialHealth;
                    }
                    printf("Monster [%s] regenerates %d health, total health now %d.\n", monster->characterName, monsterRegenAmount, monster->health);
                }

            
            }
        } 

        if (player->health <= 0) {
            player->flags &= ~FLAG_ALIVE;
            char defeatMessage[256];
            sprintf(defeatMessage, "You have been defeated in battle.");
            sendMessage(client_fd, "Narrator", "Game", defeatMessage, 1);
            printf("Player [%s] defeated in battle.\n", player->characterName);
            break;
        }
        if (allMonstersDefeated) {
            char victoryMessage[256];
            sprintf(victoryMessage, "You have defeated all monsters in the room!");
            sendMessage(client_fd, "Narrator", player->characterName, victoryMessage, 1);
            printf("Player [%s] has defeated all monsters in the room.\n", player->characterName);
        }

        
    }


    //g_list_free(monstersInRoom); 
}

Character* findCharacterByName(const char* name) {
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, characterTable);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        Character* character = (Character*)value;
        if (strcmp(character->characterName, name) == 0) {
            return character; 
        }
    }
    return NULL; // Character not found
}

void loot(int looter_fd, const char* targetName) {
    printf("Starting the loot process for target '%s' by looter with fd %d.\n", targetName, looter_fd);

    pthread_mutex_lock(&characterTableMutex);
    printf("Mutex locked for looting process.\n");

    Character* looter = retrieveCharacter(looter_fd);
    if (looter == NULL || !(looter->flags & FLAG_ALIVE)) {
        sendErrorMessage(looter_fd, ERROR_CHARACTER_NOT_FOUND, "You cannot loot right now.");
        printf("Looting failed: Looter is either not found or not alive.\n");
        pthread_mutex_unlock(&characterTableMutex);
        printf("Mutex unlocked after failed looting attempt.\n");
        return;
    }

    Character* target = findCharacterByName(targetName);
    if (target == NULL) {
        sendErrorMessage(looter_fd, ERROR_CHARACTER_NOT_FOUND, "Target not found.");
        printf("Looting failed: Target '%s' not found.\n", targetName);
        pthread_mutex_unlock(&characterTableMutex);
        printf("Mutex unlocked after failed looting attempt.\n");
        return;
    }

    if (target->flags & FLAG_ALIVE) {
        sendErrorMessage(looter_fd, ERROR_INVALID_ACTION, "Target is not lootable.");
        printf("Looting failed: Target '%s' is still alive and not lootable.\n", targetName);
        pthread_mutex_unlock(&characterTableMutex);
        printf("Mutex unlocked after failed looting attempt.\n");
        return;
    }

    printf("Transferring gold from target '%s' to looter.\n", targetName);
    looter->gold += target->gold;


    char lootMessage[256];
    sprintf(lootMessage, "You have looted %u gold from %s.", target->gold, targetName);
    sendMessage(looter_fd, looter->characterName, "Narrator", lootMessage, 1);
    printf("Loot confirmation message sent to looter.\n");

    printf("Looter now has %u gold after looting.\n", looter->gold - 100);
    target->gold = 0;

    pthread_mutex_unlock(&characterTableMutex);
    printf("Mutex unlocked after successful looting process.\n");

    printf("Loot process for target '%s' by looter with fd %d completed successfully.\n", targetName, looter_fd);
}


void broadcastMessage(const char* messageContent) {
    if (messageContent == NULL || clientsTable == NULL) return;

    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init(&iter, clientsTable);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        int client_fd = GPOINTER_TO_INT(value);

       
        sendMessage(client_fd, "All", "Server", messageContent, 1); 
    }
}

typedef struct {
    int client_fd;
} thread_data_t;

void* client_handler(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    int client_fd = data->client_fd;
    srand(time(NULL)); 
    //clientsTable = g_hash_table_new(g_str_hash, g_str_equal);


    
    version(client_fd);
    gameStart(client_fd);
    

    while(1) {
        uint8_t messageType;
        
        ssize_t readBytes = read(client_fd, &messageType, sizeof(messageType));
        if (readBytes <= 0) {
            if (readBytes == 0) {
                printf("Client disconnected\n");
            } else {
                perror("Read error");
            }
            break; 
        }
        printf("Received message of type: %d\n", messageType);

        ssize_t bytesRead;

        switch(messageType) {
                case MESSAGE_TYPE: {
                    Character* playerCharacter = retrieveCharacter(client_fd);
                    if (!(playerCharacter->flags & FLAG_ALIVE)) {
                        sendErrorMessage(client_fd, 0, "You're dead, you can't talk.");
                        continue; 
                    }

                    printf("Message received from client %d.\n", client_fd);

                    char recipientName[33] = {0};
                    char senderName[33] = {0}; 
                    char message[1025] = {0};
                    uint16_t messageLength;

                    read(client_fd, &messageLength, sizeof(messageLength));
                    messageLength = ntohs(messageLength);
                    read(client_fd, recipientName, 32);
                    read(client_fd, senderName, 32);
                    read(client_fd, message, messageLength);

                    int recipient_fd = findClientFdByName(recipientName);

                    if (recipient_fd > 0) {
                        
                        sendAcceptMessage(client_fd, MESSAGE_TYPE);
                        if (playerCharacter != NULL) {
                            sendMessage(recipient_fd, recipientName, playerCharacter->characterName, message, 0);
                        } else {
                            sendMessage(recipient_fd, recipientName, "Server", message, 0);
                        }
                        sendRoomDescription(client_fd, getCurrentRoom(playerCharacter->currentRoom));
                        listCharactersInRoom(client_fd, playerCharacter->currentRoom);
                        sendRoomConnections(client_fd, getCurrentRoom(playerCharacter->currentRoom));
                    } else {
                        printf("Recipient '%s' not found. Notifying sender.\n", recipientName);
                        char errMsg[] = "Recipient not found.";
                        if (playerCharacter != NULL) {
                            sendMessage(client_fd, playerCharacter->characterName, "Server", errMsg, 1);
                        } else {
                            sendMessage(client_fd, "unknown", "Server", errMsg, 1);
                        }
                    }

                    break;
                }

                

                case CHANGEROOM_TYPE: {
                    uint16_t newRoomNumber;
                    Character* playerCharacter = retrieveCharacter(client_fd);
                    if (!(playerCharacter->flags & FLAG_ALIVE)) {
                        sendErrorMessage(client_fd, 0, "You're dead, you can't move.");
                        continue; 
                    }

                    ssize_t bytesRead = read(client_fd, &newRoomNumber, sizeof(newRoomNumber));
                    if (bytesRead <= 0) {
                        if (bytesRead == 0) {
                            printf("Client disconnected during room change request\n");
                        } else {
                            perror("read error on room change request");
                        }
                        break;
                    }

                    newRoomNumber = newRoomNumber;

                    printf("Room change request received. New Room: %d\n", newRoomNumber);

                    char joinMsg[256];
                    snprintf(joinMsg, sizeof(joinMsg), "%s has left the room and entered room %d.", playerCharacter->characterName, newRoomNumber);
                    broadcastMessage(joinMsg);
                            
        
                    changeRoom(client_fd, newRoomNumber);

              

                    if(playerCharacter->flags & FLAG_JOIN_BATTLE && (playerCharacter->flags & FLAG_ALIVE)){
                        if (!(playerCharacter->flags & FLAG_ALIVE)) {
                            sendErrorMessage(client_fd, 0, "You're dead, you can't move.");
                            break; 
                        }
                        
                        if (playerCharacter != NULL && (playerCharacter->flags & FLAG_ALIVE)) {
                            fight(client_fd);
           
                    
                        }      
                    } 

                    

                    break;
                }
                case FIGHT_TYPE:
                    Character* playerCharacter = retrieveCharacter(client_fd);

                    printf("Fight request received from client %d.\n", client_fd);
                    if (!(playerCharacter->flags & FLAG_ALIVE)) {
                        sendErrorMessage(client_fd, 0, "You're dead, you can't fight.");
                        continue; 
                    }

                   
                    
                    if (playerCharacter != NULL && (playerCharacter->flags & FLAG_ALIVE)) {
                        fight(client_fd);
                        sendRoomDescription(client_fd, getCurrentRoom(playerCharacter->currentRoom));
                        listCharactersInRoom(client_fd, playerCharacter->currentRoom);
                       
                        sendRoomConnections(client_fd, getCurrentRoom(playerCharacter->currentRoom));
                       
                    } else {
                        sendErrorMessage(client_fd, ERROR_CHARACTER_NOT_FOUND, "Character Dead");
                    }
                    break;

                case PVPFIGHT_TYPE:
                    const char* errorMsg = "PVP Not Implemented.";
                    sendErrorMessage(client_fd, ERROR_NO_PVP, errorMsg);
                    break;

                case LOOT_TYPE: {
                    Character* playerCharacter = retrieveCharacter(client_fd);
                    if (!(playerCharacter->flags & FLAG_ALIVE)) {
                        sendErrorMessage(client_fd, 0, "You're dead, you can't loot.");
                        continue; 
                    }


                    printf("Loot request received from client %d.\n", client_fd);

                    char targetName[33]; 
                    memset(targetName, 0, sizeof(targetName)); 

                    ssize_t bytesRead = read(client_fd, targetName, 32);
                    if (bytesRead <= 0) {
                        if (bytesRead == 0) {
                            printf("Client %d disconnected unexpectedly.\n", client_fd);
                        } else {
                            perror("Failed to read loot target name from client");
                        }
                        sendErrorMessage(client_fd, ERROR_CHARACTER_NOT_FOUND, "Failed to read loot target name.");
                        break;
                    }

                    targetName[32] = '\0'; 
                    printf("Loot target name '%s' successfully read from client %d message.\n", targetName, client_fd);

                    loot(client_fd, targetName);

                    printf("Loot process for client %d targeting '%s' has been initiated.\n", client_fd, targetName);
                    sendRoomDescription(client_fd, getCurrentRoom(playerCharacter->currentRoom));
                    listCharactersInRoom(client_fd, playerCharacter->currentRoom);
                       
                    sendRoomConnections(client_fd, getCurrentRoom(playerCharacter->currentRoom));
                       
                    break;
                }


				case GAME_TYPE:
					sendRoomDescription(client_fd, mainCounter);
					break;

                case START_TYPE: {
                    printf("Start game request received.\n");

                    Character* playerCharacter = retrieveCharacter(client_fd);

                    if (playerCharacter != NULL) {
                       // if (!(playerCharacter->flags & FLAG_STARTED)) {
                            playerCharacter->flags |= FLAG_STARTED;
                            playerCharacter->flags |= FLAG_ALIVE;
                  

                            printf("%s has started the game.\n", playerCharacter->characterName);

                            char joinMsg[256];
                            snprintf(joinMsg, sizeof(joinMsg), "%s has joined the game!", playerCharacter->characterName);
                            
                            broadcastMessage(joinMsg);

                            sendAcceptMessage(client_fd, START_TYPE);
                            sendRoomDescription(client_fd, getCurrentRoom(playerCharacter->currentRoom));
                            listCharactersInRoom(client_fd, playerCharacter->currentRoom);
                            sendRoomConnections(client_fd, getCurrentRoom(playerCharacter->currentRoom));
                   /*      } else {
                            printf("Player already started the game.\n");
                        } */
                    } else {
                        const char* errorMsg = "Character not initialized. Please create a character.";
                        sendErrorMessage(client_fd, ERROR_CHARACTER_NOT_FOUND, errorMsg);
                    }

                    break;
                }



				case CHARACTER_TYPE:
					receiveCharacter(client_fd);
                  

					break;

				
				default:
					printf("Unknown message type received: %d\n", messageType);
					break;

        }
    }

    close(client_fd);
    free(data); 
    pthread_exit(NULL);
}





int main(int argc, char **argv){
	if(argc != 2){
		fprintf(stderr, "Usage: %s <port>\n", argv[0]);
		return 1;
	}
	int port = atoi(argv[1]);
	if(port <= 0){
		fprintf(stderr, "Specify port.\n");
		return 1;
	}
	struct sockaddr_in sad;
	
	sad.sin_port = htons(port);
	sad.sin_addr.s_addr = INADDR_ANY;
	sad.sin_family = AF_INET;

	int skt = socket(AF_INET, SOCK_STREAM, 0); // Step 1
	if(skt == -1){
		perror("socket");
		return 1;
	}
	if( bind(skt, (struct sockaddr*)(&sad), sizeof(struct sockaddr_in)) ){ // step 2
		perror("bind");
		return 1;
	}
	if( listen(skt, 5) ){ // step 3
		perror("listen");
		return 1;
	}	

    clientsTable = g_hash_table_new_full(client_key_hash, client_key_equal, free, NULL);

	initializeCharacterStorage();
    initializeAllRooms();
    initializeMonsters();
    while(1) {
        int client_fd;
        struct sockaddr_in client_address;
        socklen_t address_size = sizeof(struct sockaddr_in);
        client_fd = accept(skt, (struct sockaddr *)(&client_address), &address_size);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        printf("Connection made from address %s\n", inet_ntoa(client_address.sin_addr));

       
        thread_data_t* data = malloc(sizeof(thread_data_t));
        if (!data) {
            perror("Failed to allocate memory for thread data");
            close(client_fd);
            continue;
        }
        data->client_fd = client_fd;

       
        pthread_t tid;
        if (pthread_create(&tid, NULL, client_handler, (void*)data) != 0) {
            perror("Failed to create thread");
            close(client_fd);
            free(data);
            continue;
        }

        pthread_detach(tid);
    }

    close(skt);
    return 0;
}