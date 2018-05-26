#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <SFML\Network.hpp>

struct Player {
	std::string name = "tempname";
	//bool ready = false;
	//int turn; //turno de los jugadores
	//int score = 0;
	sf::TcpSocket* socket; //o algo del estilo para poder saber su port e identificarlo en la lista de clientes
	//bool answered = false;
	int lobbyID = -1;
};

struct PlayerLobby {
	std::string name = "tempname";
	bool ready = false;
	int turn; //turno de los jugadores
	int score = 0;
	sf::TcpSocket* socket; //o algo del estilo para poder saber su port e identificarlo en la lista de clientes
	bool answered = false;
	int lobbyID = -1;
};

class ScoreBoard {

	std::vector<PlayerLobby> players;
public:
	ScoreBoard() {};
	~ScoreBoard() {};

	void UpdatePlayer(PlayerLobby player); //añade/actualiza jugador
	void DeletePlayer(PlayerLobby player); //borra jugador
	std::string Winner(); //añade/actualiza jugador
};

class Lobby {
public:
	std::string name = "tempname";
	int lobbyID = -1;
	bool listed = true;	//if(players.size() >= maxPlayers || running) {listed = false;}
	bool running = false;
	bool checkWords = false;

	std::string word;
	std::string globalCurWord;
	int sizeWord = word.size();

	bool needPass = false;
	std::string pw = "1234";
	int maxPlayers = 2;
	int playerNumber; /*= players.size();*/
	int curTurn;
	int maxTurns;	//maxTurns = playerNumber * turnMultiplier;
	int turnMultiplier;
	
	bool gameStarted = false;
	bool startNewTurn = false;

	std::vector<PlayerLobby*> players;
	PlayerLobby* lobbyPlayerPtr = new PlayerLobby;
	ScoreBoard scoreboard;
	//GESTIÓN DEL READY
	/*TODO*/
	int remainingPlayers;

public:
	//Methods	
	void SendToAll(sf::Packet); //Send message to all. The logic being that if a player wants to send something
							 //	(message, image, etc...) it has to be sent always to everyone by the server.
							 //This allows us to send messages to all players in single lobbies only knowing
							 //	the lobby the players comes from. We will need a method to check if that player
							 //	is in said lobby or not.
							 //line 158 (recieve MSG if(wordCorrect))
							 //line 177 (recieve MSG if(sendWord))
							 //line 281 (recieve IMG)
							 //line 294 (recieve TIM)

	void SendImage(sf::Packet);

	bool IsInLobby(std::string playerName); //Checks inside the vector of players inside the lobby to check
												   // if the player is in it.

	void PrintLobby(); //Method that prints on console any important information when listing all the lobbies.
							  //Also checks if it should print itself: if(listed) {/*PRINT*/}

	int RemainingReady() {
		int readyCount = this->maxPlayers;
		std::cout << "Max players remaining ready: " << this->maxPlayers << std::endl;
		for (int indexASD = 0; indexASD < this->players.size(); indexASD++) {
			if (this->players[indexASD]->ready) readyCount--;
			std::cout << "player " << this->players[indexASD]->name << " ready as " << this->players[indexASD]->ready << std::endl;
		}
		return readyCount;
	}
	void DetectPlayerPainting(); //encuentra player segun turno

	void DetectPlayer(unsigned short port);

	void JoinPlayer(Player playerThatJoined); //Method that adds a player to the players vector and notifies to rest of players that he joined. 

	void DisconnectAll(); //Method that disconnects all players' sockets. After this the server deletes this lobby from the lobby list.
};