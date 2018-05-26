#include "scoreboard.h"

bool cmpFunc(PlayerLobby a, PlayerLobby b) { return a.score > b.score; }

void ScoreBoard::UpdatePlayer(PlayerLobby player) {
	bool playerFound = false;
	if (players.size() > 0) {
		//buscar si el nombre ya esta usado y actualizar puntos. Si no, añadirlo
		for (int i = 0; i < players.size(); i++) {
			if (strcmp(player.name.c_str(), players[i].name.c_str()) == 0) {
				players[i].score = player.score;
				playerFound = true;
				break;
			}
		}
	}

	if (!playerFound) {
		players.push_back(player); //añadir player si es nuevo
	}

	//ordenar
	std::sort(players.begin(), players.end(), cmpFunc);
}
void ScoreBoard::DeletePlayer(PlayerLobby player) {
	bool playerFound = false;
	//buscar si el nombre ya esta usado y borrarlo
	for (int i = 0; i < players.size(); i++) {
		if (strcmp(player.name.c_str(), players[i].name.c_str()) == 0) {
			players.erase(players.begin() + i);
			playerFound = true;
			break;
		}
	}
	
	/*if (!playerFound) {
		players.push_back(player); //añadir player si es nuevo
	}*/

	//ordenar
	std::sort(players.begin(), players.end(), cmpFunc);
}
std::string ScoreBoard::Winner() {
	return players[0].name;
}


void Lobby::SendToAll(sf::Packet packet) {
	for (int i = 0; i < this->players.size(); i++) {
		this->players[i]->socket->send(packet);
	}
}

void Lobby::SendImage(sf::Packet packet) {	//When calling this function lobbyPlayerPtr is already set to the player that is drawing.
	for (int i = 0; i < this->players.size(); i++) {
		if (this->lobbyPlayerPtr->socket->getRemotePort() != this->players[i]->socket->getRemotePort()) {	//Si el port es diferente al del player que pinta
			this->players[i]->socket->send(packet);
			std::cout << "IMAGE SENT" << std::endl;
		}
	}
}

void Lobby::DetectPlayerPainting() { //encuentra player segun turno
	bool found = false;
	int i = 0;
	Player* player = new Player;
	while (!found && i < this->players.size()) {
		if (this->players[i]->turn == this->curTurn % this->playerNumber) {
			this->lobbyPlayerPtr = this->players[i];
			found = true;
		}
		i++;
	}
}
void Lobby::DetectPlayer(unsigned short port) { //encuentra player segun turno
	bool found = false;
	int i = 0;
	while (!found && i < this->players.size()) {
		if (this->players[i]->socket->getRemotePort() == port) {
			this->lobbyPlayerPtr = this->players[i];
			found = true;
		}
		i++;
	}
}