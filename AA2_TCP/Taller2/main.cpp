#include <iostream>
#include <list>
#include "scoreboard.h"
enum commands { NOM, DEN, CON, INF, MSG, IMG, WRD, GUD, BAD, WNU, WIN, DIS, END, RNK, RDY, TIM, CRE, COK, CNO, LIS, JOI, JOK, JNO };

std::vector<std::string> wordsVector{"coche", "robot", "camara", "pelota", "gafas", "libro", "piramide", "pistola", "gato", "caballo", "huevo", "gallina", "sombrero",
"pokemon", "mario", "sonic", "pacman", "tetris"};

Player* globalPlayerPtr = new Player;	//This pointer points to the player that send the message on the GeneralPlayer.
Lobby* globalLobbyPtr = new Lobby;		//This pointer points to the lobby of the player that sends the message.	globalLobbyPtr->globalPlayerPtr points to the same player but inside the lobby.

std::vector<Lobby*> lobbies = std::vector<Lobby*>();
int augmentingLobbyID = 0;
/*
int RemainingReady(std::vector<Player*> players) {
	int readyCount = players.size();
	int i = 0;
	for (int i = 0; i < players.size(); i++) {
		if (players[i]->ready) readyCount--;
	}
	return readyCount;
}*/

void DetectLobby(int lobbyID, sf::TcpSocket& sock) {
	//encontrar lobby
	bool found = false;
	int i = 0;
	while (!found && i < lobbies.size()) {
		if (lobbies[i]->lobbyID == lobbyID) {
			globalLobbyPtr = lobbies[i];
			globalLobbyPtr->DetectPlayer(sock.getRemotePort());
			found = true;
		}
		i++;
	}
}
void DetectPlayer(sf::TcpSocket& client, std::vector<Player*> players) { //encuentra player segun socket, dado que hara falta en varias ocasiones creo que sera util
	//encontrar player comparando remoteport
	bool found = false;
	int i = 0;
	while (!found && i < players.size()) {
		if (players[i]->socket->getRemotePort() == client.getRemotePort()) {
			globalPlayerPtr = players[i];
			if(players[i]->lobbyID != -1) DetectLobby(players[i]->lobbyID, client);	//-1 es el set de base de los players cuando se crean en el connect. Si no tiene asignado un lobby este ID será -1. Se tiene que setear a -1 cuando se le devuelve al lobby.
			found = true;
		}
		i++;
	}
}

//void DetectPlayer(int turn, std::vector<Player*> players) { //encuentra player segun turno
//	bool found = false;
//	int i = 0;
//	Player* player = new Player;
//	while (!found && i < players.size()) {
//		if (players[i]->turn == turn) {
//			globalPlayerPtr = players[i];
//			found = true;
//		}
//		i++;
//	}
//}
std::string PickWord() { //elige palabra random de la lista
	std::string wordPicked = "platano"; //default
	if (wordsVector.size() > 0) {
		int randomPick = rand() % wordsVector.size();
		wordPicked = wordsVector[randomPick];
		//wordsVector.erase(wordsVector.begin() + randomPick); //asiegura que no se repitan palabras, cuando no hayan mas devolveria platano que es el default
	}
	return wordPicked;
}
void ControlServidor()
{
	bool running = true;
	//bool gameStarted = false;
	//bool startNewTurn = false;
	//int curTurn = 0; //turno actual
	//int playerNumber; //players.size() shortcut
	//int maxTurns;
	//bool checkWords = false;
	//std::string globalCurWord;
	//ScoreBoard scoreboard;
	// Create a socket to listen to new connections
	sf::TcpListener listener;
	sf::Socket::Status status = listener.listen(50000);
	if (status != sf::Socket::Done)	{
		std::cout << "Error al abrir listener\n";
		exit(0);
	}
	// Create a list to store the future clients
	std::list<sf::TcpSocket*> clients;
	//vec of players
	std::vector<Player*> globalPlayers;
	// Create a selector
	sf::SocketSelector selector;
	// Add the listener to the selector
	selector.add(listener);

	// Endless loop that waits for new connections
	while (running)	{
		
		// Make the selector wait for data on any socket
		if (selector.wait()) {
			// Test the listener
			if (selector.isReady(listener)) {
				// The listener is ready: there is a pending connection
				sf::TcpSocket* client = new sf::TcpSocket;
				if (listener.accept(*client) == sf::Socket::Done)
				{

					// Add the new client to the clients list
					std::cout << "Llega el cliente con puerto: " << client->getRemotePort() << std::endl;
					clients.push_back(client);
					//creamos new player
					Player* player = new Player;
					player->socket = client;
					globalPlayers.push_back(player);
					// Add the new client to the selector so that we will
					// be notified when he sends something
					selector.add(*client);
				}
				else
				{
					// Error, we won't get a new connection, delete the socket
					std::cout << "Error al recoger conexión nueva\n";
					delete client;
				}
			}
			else {	//Receive:
				// The listener socket is not ready, test all other sockets (the clients)
				//lista de iteradores que iteran listas de tcp sockets
				std::list<std::list<sf::TcpSocket*>::iterator> itTemp; //si se desconecta un socket, lo guardamos en este it para borrarlo después
				for (std::list<sf::TcpSocket*>::iterator it = clients.begin(); it != clients.end(); ++it) {
					sf::TcpSocket& client = **it;
					if (selector.isReady(client)) {
						// The client has sent some data, we can receive it
						sf::Packet packet;
						status = client.receive(packet);

						if (status == sf::Socket::Done)	{

							int command;
							if (packet >> command) {//inicializa cosas antes del switch

								std::string strRec;
								std::string word;
								bool used = false;
								sf::Packet newPacket;
								sf::Packet imagePacket;
								int _w, _h;
								int arraySize;
								int remainingPlayers;
								DetectPlayer(client, globalPlayers); //identifica al player
								bool sendWord = true; //envia mensaje o no
								switch (command) {
								case commands::MSG:
									packet >> strRec;
									std::cout << "He recibido " << strRec << " del puerto " << client.getRemotePort() << std::endl;

									//checkear si el msg es correcto
									if (globalLobbyPtr->checkWords) {
										std::cout << "CHECKING WORDS" << std::endl;
										std::string tempWord = " >" + globalLobbyPtr->word;
										std::cout << "Palabra correcta: " << tempWord << std::endl;
										std::cout << "Palabra enviada: " << strRec << std::endl;
										PlayerLobby* wordPlayer = globalLobbyPtr->lobbyPlayerPtr; //nos guardamos quien esta escribiendo
										globalLobbyPtr->DetectPlayerPainting(); // saber quien esta pintando
										if (strcmp(tempWord.c_str(), strRec.c_str()) == 0) { //comparar que sea la palabra correcta
											std::cout << "Palabra enviada: " << strRec << std::endl;
											sendWord = false; //no enviar palabra correcta al chat
											//comprobar si ha sido dibujante o no
											if (wordPlayer->turn != globalLobbyPtr->lobbyPlayerPtr->turn && !wordPlayer->answered) { //asegurarse que no se repitan
												std::cout << "NO HABIA RESPONDIDO ANTES " << wordPlayer->answered << " Y NO ES EL QUE PINTA" << wordPlayer->name << std::endl;
												//gud al jugador, supongo que habria que calcular puntos
												wordPlayer->answered = true;
												wordPlayer->score += 1;
												globalLobbyPtr->scoreboard.UpdatePlayer(*wordPlayer);
												newPacket << commands::GUD;
												client.send(newPacket);
												/*//CAMBIAR PARA QUE SE MANDE A LOS DEL LOBBY: globalLobbyPtr->SendToAll(packet)
												for (std::list<sf::TcpSocket*>::iterator it2 = clients.begin(); it2 != clients.end(); ++it2) {
													sf::TcpSocket& tempSok = **it2;

													newPacket.clear();
													newPacket << commands::WIN << wordPlayer->name << wordPlayer->score;
													tempSok.send(newPacket);
												}*/

												//ENVÍO DE PAQUET DE ACIERTO A TODOS LOS JUGADORES DENTRO DEL LOBBY
												newPacket.clear();
												newPacket << commands::WIN << wordPlayer->name << wordPlayer->score;
												globalLobbyPtr->SendToAll(newPacket);
											}
										}
										else {
											//bad
											if (wordPlayer->turn != globalLobbyPtr->lobbyPlayerPtr->turn) { //comprobar que no sea al que ha pintado
												newPacket << commands::BAD;
												client.send(newPacket);
											}
										}

									}
									//Reenviar mensaje a todos los clientes: //CAMBIAR PARA QUE SE MANDE A LOS DEL LOBBY: globalLobbyPtr->SendToAll(packet)
									if (sendWord) {
										/*for (std::list<sf::TcpSocket*>::iterator it2 = clients.begin(); it2 != clients.end(); ++it2) {
											newPacket.clear();
											sf::TcpSocket& tempSok = **it2;

											newPacket << commands::MSG << globalPlayerPtr->name << strRec;
											tempSok.send(newPacket);
										}*/
										//SE MANDA EL MENSAJE A TODOS LOS JUGADORES DEL LOBBY
										newPacket.clear();
										newPacket << commands::MSG << globalPlayerPtr->name << strRec;
										globalLobbyPtr->SendToAll(newPacket);
									}

									break;
								case NOM:
									//compara con el resto de players si ya está usado o no
									packet >> strRec;
									for (int i = 0; i < globalPlayers.size(); i++) {
										if (strcmp(globalPlayers[i]->name.c_str(), strRec.c_str()) == 0) {
											used = true;
										}
									}
									//si no está usado --> send CON y añade, si lo está --> send DEN
									if (!used) {
										globalPlayerPtr->name = strRec;
										newPacket << commands::CON;
										globalPlayerPtr->socket->send(newPacket);
										//creo y envío paquete LIS
										sf::Packet lisPacket;
										lisPacket << commands::LIS;
										int lobSize = lobbies.size();
										lisPacket << lobSize;
										std::cout << "sending LIS " << lobbies.size() << std::endl;
										for (int i = 0; i < lobbies.size(); i++) {
											lisPacket << lobbies[i]->name;
											lisPacket << lobbies[i]->lobbyID;
											lisPacket << lobbies[i]->needPass;
											lisPacket << lobbies[i]->maxPlayers;
											int playerSize = lobbies[i]->players.size();
											lisPacket << playerSize;
											std::cout << i << ":  " << lobbies[i]->name << " " << lobbies[i]->maxPlayers << " " << lobbies[i]->needPass << std::endl;
										}
										client.send(lisPacket);
										//CAMBIADO PARA QUE SOLO MANDE A LOS DE SU LOBBY EN EL JOI ========================================================================================================================= VA AL NUEVO COMANDO DE JOIN LOBBY
										/*//avisamos a todos que se ha conectado un nuevo cliente
										for (std::list<sf::TcpSocket*>::iterator it = clients.begin(); it != clients.end(); ++it) {
											sf::TcpSocket& tempSok = **it;
											std::string playerStr = globalPlayerPtr->name;
											sf::Packet packet;
											packet << commands::INF << playerStr;
											tempSok.send(packet);
										}*/
									}
									else {
										newPacket << commands::DEN;
										client.send(newPacket);
									}
									break;
								case RDY:
									//IDENTIFICAR QUÉ LOBBY ESTAMOS
									if (!globalLobbyPtr->gameStarted) {
										globalLobbyPtr->lobbyPlayerPtr->ready = true;
										std::cout << globalLobbyPtr->lobbyPlayerPtr->name << std::endl;
										globalLobbyPtr->remainingPlayers = globalLobbyPtr->RemainingReady();
			//CAMBIAR PARA QUE SOLO MANDE A LOS DE SU LOBBY =========================================================================================================================
										/*//avisamos a todos que el jugador está ready
										for (std::list<sf::TcpSocket*>::iterator it = clients.begin(); it != clients.end(); ++it) {
											sf::TcpSocket& tempSok = **it;
											sf::Packet packet;
											if (globalLobbyPtr->remainingPlayers > 0) {
												std::string playerStr = globalPlayerPtr->name;

												packet << commands::MSG << "EL JUGADOR " + playerStr + " ESTÁ PREPARADO PARA JUGAR (FALTA(n) " + std::to_string(remainingPlayers)
													+ " READY(s) PARA EMPEZAR)";
												tempSok.send(packet);
											}
											else if (globalLobbyPtr->players.size() == 1) {
												packet << commands::MSG << "FALTAN MÁS JUGADORES PARA PODER JUGAR";
												tempSok.send(packet);
											}
											else {
												packet << commands::MSG << "EMPIEZA LA PARTIDA";
												tempSok.send(packet);

												globalLobbyPtr->gameStarted = true;
											}
										}*/

										for (int playersLobbyIndex = 0; playersLobbyIndex < globalLobbyPtr->players.size(); playersLobbyIndex++) {
											//sf::TcpSocket& tempSok = **it;
											sf::Packet packet;
											if (globalLobbyPtr->remainingPlayers > 0) {
												std::string playerStr = globalPlayerPtr->name;

												packet << commands::MSG << "EL JUGADOR " + playerStr + " ESTÁ PREPARADO PARA JUGAR (FALTA(n) " + std::to_string(globalLobbyPtr->remainingPlayers)
													+ " READY(s) PARA EMPEZAR)";
												globalLobbyPtr->players[playersLobbyIndex]->socket->send(packet);
											}
											else if (globalLobbyPtr->players.size() == 1) {
												packet << commands::MSG << "FALTAN MÁS JUGADORES PARA PODER JUGAR";
												globalLobbyPtr->players[playersLobbyIndex]->socket->send(packet);
											}
											else {
												packet << commands::MSG << "EMPIEZA LA PARTIDA";
												globalLobbyPtr->players[playersLobbyIndex]->socket->send(packet);

												globalLobbyPtr->gameStarted = true;
											}
										}

										//ya que el juego va a empezar, aprovechamos para setear todo lo necesario y turno 1
										if (globalLobbyPtr->gameStarted) {
											globalLobbyPtr->curTurn = 0;
											globalLobbyPtr->playerNumber = globalLobbyPtr->players.size();
											globalLobbyPtr->maxTurns = globalLobbyPtr->playerNumber * globalLobbyPtr->turnMultiplier;	//CAMBIAR EL 2 POR EL PARÁMETRO SETEADO EN EL CREATE (DONE)
											//crear orden de los turnos
											globalLobbyPtr->word = PickWord();
											globalLobbyPtr->globalCurWord = word;
											std::cout << "GAME STARTED: CURR WORLD" << globalLobbyPtr->globalCurWord << std::endl;
											globalLobbyPtr->sizeWord = globalLobbyPtr->word.size();
											for (int i = 0; i < globalLobbyPtr->playerNumber; i++) {
												globalLobbyPtr->players[i]->turn = i;
												if (i == 0) { sf::Packet turnPacket; turnPacket << commands::WRD << globalLobbyPtr->word;  globalLobbyPtr->players[i]->socket->send(turnPacket); }
												else { sf::Packet turnPacket; turnPacket << commands::WNU << globalLobbyPtr->players[0]->name << globalLobbyPtr->sizeWord;  globalLobbyPtr->players[i]->socket->send(turnPacket); }
												
												globalLobbyPtr->scoreboard.UpdatePlayer(*globalLobbyPtr->players[i]);
											}
										}
									}
									break;
								case IMG:
									/*
									recibe la imagen dibujada por el cliente que ha dibujado
										RECIBES Y REENVIAS EL MISMO PACKETE SIN MÁS
									quizá podria aprovechar y reenviar la imagen al resto, dependiendo de al final como va le code xd
									*/
									std::cout << "IMAGE RECEIVED" << std::endl;

									packet >> _w;
									packet >> _h;
									arraySize = _w * _h * 4;

									imagePacket << commands::IMG;
									imagePacket << _w << _h;
									for (int index = 0; index < arraySize; index++) {
										sf::Uint8 tempUint;
										packet >> tempUint;
										imagePacket << tempUint;
									}
									globalLobbyPtr->DetectPlayerPainting();	//Detect current player that is drawing.
									//CAMIAR PARA QUE SE HAGA INTERNO AL LOBBY.s
									globalLobbyPtr->SendImage(imagePacket);
									/*for (std::list<sf::TcpSocket*>::iterator it = clients.begin(); it != clients.end(); ++it) {
										sf::TcpSocket& tempSok = **it;

										if (globalLobbyPtr->lobbyPlayerPtr->socket->getRemotePort() != tempSok.getRemotePort()) {	//Only send image to everyone except for the drawer. Which is already drawing the image.
											tempSok.send(imagePacket);
											std::cout << "IMAGE SENT" << std::endl;
										}
									}*/
									globalLobbyPtr->checkWords = true; //start checking words once
									break;
								case TIM:
									std::cout << "TIM RECIEVED" << std::endl;
									//acaba el tiempo para los players y empieza un turno nuevo
									globalLobbyPtr->checkWords = false; //stop checking words
									newPacket.clear();
									newPacket << commands::TIM;
									globalLobbyPtr->SendToAll(newPacket);
									/*
									for (std::list<sf::TcpSocket*>::iterator it2 = clients.begin(); it2 != clients.end(); ++it2) {
										sf::TcpSocket& tempSok = **it2;
										newPacket << commands::TIM;
										tempSok.send(newPacket);
									}*/
									globalLobbyPtr->startNewTurn = true;

									break;
								case DIS:
									/*
									quita al señor de players, clients y avisa al resto supongo lel
									*/
									break;

								case JOI: {	//FALTA HACER QUE EL PLAYER SE AÑADA AL LOBBY Y SE NOTIFIQUE (DONE)
									std::cout << "received JOI" << std::endl;
									int desiredLobbyID = -1;
									packet >> desiredLobbyID;
									for (int lbbyIndx = 0; lbbyIndx < lobbies.size(); lbbyIndx++) {
										if (lobbies[lbbyIndx]->lobbyID == desiredLobbyID) {
											//COMPROBAR SI ESTÁ LLENO O NO
											if (lobbies[lbbyIndx]->players.size() < lobbies[lbbyIndx]->maxPlayers) {	//POSIBLE: CAMBIAR playerNumber por player.size()
												//MIRAR QUE NECESITE PASS O NO
												if (lobbies[lbbyIndx]->needPass) {
													std::string playerPass = "";
													packet >> playerPass;
													//COMPROBAR PASS
													if (strcmp(playerPass.c_str(), lobbies[lbbyIndx]->pw.c_str()) == 0) {
														//PASS CORRECTO JOK
														//JOIN
														globalPlayerPtr->lobbyID = desiredLobbyID;

														sf::Packet packOK;
														packOK << commands::JOK;
														sf::Packet packInf;
														packInf << commands::INF;
														packInf << globalPlayerPtr->name;

														globalPlayerPtr->socket->send(packOK);
														lobbies[lbbyIndx]->SendToAll(packInf);
														
														PlayerLobby* tempNewPlayerLobby = new PlayerLobby;	//Igualar el socket/nombre/lobbyID al del vector general.
														tempNewPlayerLobby->socket = globalPlayerPtr->socket;
														tempNewPlayerLobby->name = globalPlayerPtr->name;
														tempNewPlayerLobby->lobbyID = desiredLobbyID;
														globalPlayerPtr->lobbyID = desiredLobbyID;
														lobbies[lbbyIndx]->players.push_back(tempNewPlayerLobby);
														//lobbies[lbbyIndx]->playerNumber++;							//POSIBLE: CAMBIAR playerNumber por player.size()
													}
													else { //contraseña errónea
														//JNO
														sf::Packet packNO;
														packNO << commands::JNO;
														globalPlayerPtr->socket->send(packNO);
													}
												} 
												else { //join directo
													//JOIN JOK
													globalPlayerPtr->lobbyID = desiredLobbyID;

													sf::Packet packOK;
													packOK << commands::JOK;
													sf::Packet packInf;
													packInf << commands::INF;
													packInf << globalPlayerPtr->name;

													globalPlayerPtr->socket->send(packOK);
													lobbies[lbbyIndx]->SendToAll(packInf);
													
													PlayerLobby* tempNewPlayerLobby = new PlayerLobby;	//Igualar el socket/nombre/lobbyID al del vector general.
													tempNewPlayerLobby->socket = globalPlayerPtr->socket;
													tempNewPlayerLobby->name = globalPlayerPtr->name;
													tempNewPlayerLobby->lobbyID = desiredLobbyID;
													globalPlayerPtr->lobbyID = desiredLobbyID;
													lobbies[lbbyIndx]->players.push_back(tempNewPlayerLobby);
													//lobbies[lbbyIndx]->playerNumber++;
												}
											}
											else { //sala llena
												//JNO
												sf::Packet packNO;
												packNO << commands::JNO;
												globalPlayerPtr->socket->send(packNO);
											}

										}
									}
									break;
								}
								case commands::CRE: {
									std::string desiredLobbyName = "";
									int desiredMaxPlayers = 2;
									int desiredMaxTurns = 2;
									bool passWanted = false;
									std::string desiredPassword = "1234";

									packet >> desiredLobbyName;
									bool nameTaken = false;
									std::cout << "received CRE" << std::endl;
									if (lobbies.size() > 0) {
										for (int lobbiesIndex = 0; lobbiesIndex < lobbies.size(); lobbiesIndex++) {
											if (strcmp(lobbies[lobbiesIndex]->name.c_str(), desiredLobbyName.c_str()) == 0) { nameTaken = true; }
										}

										if (nameTaken) {
											//ENVIAR CNO
											sf::Packet cnoPacket;
											cnoPacket << commands::CNO;
											globalPlayerPtr->socket->send(cnoPacket);
											std::cout << "CNO sent" << std::endl;
										} else {
											//COK
											packet >> desiredMaxPlayers;
											packet >> desiredMaxTurns;
											packet >> passWanted;
											if (passWanted) {
												packet >> desiredPassword;
												//CREAR LOBBY CON PASSWORD
												Lobby* tempLobby = new Lobby;
												tempLobby->name = desiredLobbyName;
												tempLobby->needPass = true;
												tempLobby->pw = desiredPassword;
												tempLobby->lobbyID = augmentingLobbyID;
												tempLobby->maxPlayers = desiredMaxPlayers;
												tempLobby->turnMultiplier = desiredMaxTurns;

												lobbies.push_back(tempLobby);
											}
											else {
												//CREAR LOBBY SIN PASSWORD
												Lobby* tempLobby = new Lobby;
												tempLobby->name = desiredLobbyName;
												tempLobby->needPass = false;
												tempLobby->lobbyID = augmentingLobbyID;
												tempLobby->maxPlayers = desiredMaxPlayers;
												tempLobby->turnMultiplier = desiredMaxTurns;

												lobbies.push_back(tempLobby);
											}
											
											//ENVIAR COK
											sf::Packet cokPacket;
											cokPacket << commands::COK;
											cokPacket << augmentingLobbyID;
											globalPlayerPtr->socket->send(cokPacket);
											PlayerLobby* tempNewPlayerLobby = new PlayerLobby;	//Igualar el socket/nombre/lobbyID al del vector general.
											tempNewPlayerLobby->socket = globalPlayerPtr->socket;
											tempNewPlayerLobby->name = globalPlayerPtr->name;
											tempNewPlayerLobby->lobbyID = augmentingLobbyID;
											globalPlayerPtr->lobbyID = augmentingLobbyID;
											lobbies[int(lobbies.size()) - 1]->players.push_back(tempNewPlayerLobby);
											//lobbies[lbbyIndx]->playerNumber++;
											augmentingLobbyID++;
											std::cout << "COK sent with " << lobbies.size() << " lobbies" << std::endl;

										}
									}
									else { //primer create - retocar
										//COK
										packet >> desiredMaxPlayers;
										packet >> desiredMaxTurns;
										packet >> passWanted;
										if (passWanted) {
											packet >> desiredPassword;
											//CREAR LOBBY CON PASSWORD
											Lobby* tempLobby = new Lobby;
											tempLobby->name = desiredLobbyName;
											tempLobby->needPass = true;
											tempLobby->pw = desiredPassword;
											tempLobby->lobbyID = augmentingLobbyID;
											tempLobby->maxPlayers = desiredMaxPlayers;
											tempLobby->turnMultiplier = desiredMaxTurns;

											lobbies.push_back(tempLobby);
										}
										else {
											//CREAR LOBBY SIN PASSWORD
											Lobby* tempLobby = new Lobby;
											tempLobby->name = desiredLobbyName;
											tempLobby->needPass = false;
											tempLobby->lobbyID = augmentingLobbyID;
											tempLobby->maxPlayers = desiredMaxPlayers;
											tempLobby->turnMultiplier = desiredMaxTurns;

											lobbies.push_back(tempLobby);
										}

										//ENVIAR COK
										sf::Packet cokPacket;
										cokPacket << commands::COK;
										cokPacket << augmentingLobbyID;
										globalPlayerPtr->socket->send(cokPacket);

										PlayerLobby* tempNewPlayerLobby = new PlayerLobby;	//Igualar el socket/nombre/lobbyID al del vector general.
										tempNewPlayerLobby->socket = globalPlayerPtr->socket;
										tempNewPlayerLobby->name = globalPlayerPtr->name;
										tempNewPlayerLobby->lobbyID = augmentingLobbyID;
										globalPlayerPtr->lobbyID = augmentingLobbyID;
										std::cout << "Augmenting lobby id " << augmentingLobbyID << std::endl;
										lobbies[int(lobbies.size()) - 1]->players.push_back(tempNewPlayerLobby);
										std::cout << "Added player who created with port: " << globalPlayerPtr->socket->getRemotePort() << " | " << tempNewPlayerLobby->socket->getRemotePort() << std::endl;
										//lobbies[lbbyIndx]->playerNumber++;
										//std::cout << lobbies[int(lobbies.size()) - 1]->players[0]->name << std::endl;
										augmentingLobbyID++;
										std::cout << "COK sent" << std::endl;
										std::cout << "COK sent with " << lobbies.size() << " lobbies" << std::endl;
									}

									break;
								}
								}	//</Switch>
							}
						}
						else if (status == sf::Socket::Disconnected) {

							DetectPlayer(client, globalPlayers); //identifica al player
							globalLobbyPtr->DetectPlayer(globalPlayerPtr->socket->getRemotePort()); 
							if (globalLobbyPtr->lobbyPlayerPtr->turn == globalLobbyPtr->curTurn % globalLobbyPtr->playerNumber) globalLobbyPtr->startNewTurn = true;  //si es el que pinta empieza nuevo turno
							if (globalLobbyPtr->startNewTurn) std::cout << "el k dibujaba se ha desconectado" << std::endl;

							for (std::list<sf::TcpSocket*>::iterator it2 = clients.begin(); it2 != clients.end(); ++it2) {
								sf::TcpSocket& tempSok = **it2;
								sf::Packet newPacket;
								newPacket << commands::DIS << globalPlayerPtr->name << globalPlayerPtr->name;
								tempSok.send(newPacket);
							}

							selector.remove(client);
							itTemp.push_back(it); //guardamos iterador del socket desconectado

					//FALTA PONER QUE SE BORRE DEL VECTOR GENERAL DE PLAYERS EN EL MAIN.
							bool found = false;
							int i = 0;
							while (!found && i < globalLobbyPtr->players.size()){
								if (globalLobbyPtr->players[i]->socket->getRemotePort() == client.getRemotePort()) {

									globalLobbyPtr->scoreboard.DeletePlayer(*globalLobbyPtr->lobbyPlayerPtr);
									globalLobbyPtr->players.erase(globalLobbyPtr->players.begin() + i);
									if (globalLobbyPtr->players.size() == 1) {
										sf::Packet packet;
										packet << commands::END << "";
										globalLobbyPtr->players[0]->socket->send(packet);
										//running = false;	//NO DEBERÏA CERRARSE
									}
									found = true;
								}
								i++;
							}
							std::cout << "Elimino el socket que se ha desconectado\n";
						}
						else {
							std::cout << "Error al recibir de " << client.getRemotePort() << std::endl;
						} //</RECIEVE>

						//simular turno nuevo (hacer que el juego se acabe al llegar a max turns)
						if (globalLobbyPtr->startNewTurn) {
							globalLobbyPtr->startNewTurn = false;
							for (int i = 0; i < globalLobbyPtr->players.size(); i++) {
								globalLobbyPtr->players[i]->answered = false;
							}
							bool nextTurnPossible = false;
							while (!nextTurnPossible) {
								globalLobbyPtr->curTurn++;
								globalLobbyPtr->DetectPlayerPainting();
								std::cout << "POSSIBLE NEXT PLAYER PAINTING: " << globalLobbyPtr->lobbyPlayerPtr->name << std::endl;
								if (globalLobbyPtr->lobbyPlayerPtr->turn == globalLobbyPtr->curTurn % globalLobbyPtr->playerNumber) { //comprueba que corresponda el turno con el jugador, por si esta desconectado
									nextTurnPossible = true;
								}
							}
							std::cout << "NEXT PLAYER PAINTING: " << globalLobbyPtr->lobbyPlayerPtr->name << std::endl;
							if (globalLobbyPtr->curTurn < globalLobbyPtr->maxTurns) {
								std::cout << "Turn: " << globalLobbyPtr->curTurn << std::endl;
								globalLobbyPtr->word = PickWord(); //pick a word
								globalLobbyPtr->globalCurWord = globalLobbyPtr->word;
								globalLobbyPtr->sizeWord = globalLobbyPtr->word.size();
								for (int i = 0; i < globalLobbyPtr->players.size(); i++) {
									if (globalLobbyPtr->players[i]->turn == globalLobbyPtr->lobbyPlayerPtr->turn) { sf::Packet turnPacket; turnPacket << commands::WRD << globalLobbyPtr->word;  globalLobbyPtr->players[i]->socket->send(turnPacket); }
									else { sf::Packet turnPacket; turnPacket << commands::WNU << globalLobbyPtr->lobbyPlayerPtr->name << globalLobbyPtr->sizeWord;  globalLobbyPtr->players[i]->socket->send(turnPacket); }
								}
								//CAMBIADO PARA MANDARLO A LOS DEL LOBBY SOLO
								/*for (std::list<sf::TcpSocket*>::iterator it = clients.begin(); it != clients.end(); ++it) {
									sf::TcpSocket& tempSok = **it;
									std::string playerStr = globalPlayerPtr->name;
									sf::Packet packet;
									if (globalPlayerPtr->socket->getRemotePort() == tempSok.getRemotePort()) {
										packet << commands::WRD << globalLobbyPtr->word;
										tempSok.send(packet);
									}
									else {
										globalLobbyPtr->sizeWord = globalLobbyPtr->word.size();
										packet << commands::WNU << playerStr << globalLobbyPtr->sizeWord;
										tempSok.send(packet);
									}
								}*/
							}
							else {
					//CAMBIAR PARA MANDARLO A LOS DEL LOBBY SOLO
								for (std::list<sf::TcpSocket*>::iterator it2 = clients.begin(); it2 != clients.end(); ++it2) {
									sf::TcpSocket& tempSok = **it2;
									sf::Packet newPacket;
									newPacket << commands::END << globalLobbyPtr->scoreboard.Winner();
									tempSok.send(newPacket);
									//running = false;	//NO QUEREMOS QUE SE PARE.
								}
							}
						}
					}
				}
				//iterador que itera la lista de iteradores que iteran la lista de sockets
				for (std::list<std::list<sf::TcpSocket*>::iterator>::iterator it = itTemp.begin(); it != itTemp.end(); ++it) {
					clients.erase(*it); 
				}
				//std::list<std::list<sf::TcpSocket*>::iterator>::swap(itTemp);
			}
		}
		/*if (gameStarted && curTurn < maxTurns) {
			//simular turnos
			std::cout << "Turn: " << curTurn << std::endl;
			DetectPlayer(curTurn % playerNumber, players);
			std::string wordStr = "patata"; //pick a word
			for (std::list<sf::TcpSocket*>::iterator it = clients.begin(); it != clients.end(); ++it) {
				sf::TcpSocket& tempSok = **it;
				std::string playerStr = globalPlayerPtr->name;
				sf::Packet packet;
				if (globalPlayerPtr->socket->getRemotePort() == tempSok.getRemotePort()) {
					packet << commands::WRD << wordStr;
					tempSok.send(packet);
				}
				else {
					int sizeWord = wordStr.size();
					packet << commands::WNU << playerStr << sizeWord;
					tempSok.send(packet);
				}
			}
			curTurn++;
			system("pause");
		}
		*/
	}
}

void main()
{
	std::cout << "Seras servidor (s)? ... ";
	char c;
	std::cin >> c;

	if (c == 's') {
		ControlServidor();
	}
	else {
		delete globalPlayerPtr;
		exit(0);
	}
	delete globalPlayerPtr;

}