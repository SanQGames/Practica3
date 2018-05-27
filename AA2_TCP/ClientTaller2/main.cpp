#include <SFML\Graphics.hpp>
#include <SFML\Network.hpp>
#include <cstring>
#include <mutex>
#include <thread>
#include <time.h>
#include "scoreboard.h"
#include "Circle.h"
#include "Chronometer.h"
#define MAX_MENSAJES 25
#define SET 0
#define GET 1

std::vector<std::string> aMensajes;
std::mutex myMutex;
bool connected = false;
std::thread receiveThread;
sf::IpAddress ip = sf::IpAddress::getLocalAddress();
sf::TcpSocket socket;
char connectionType, mode;
char buffer[2000];
std::size_t received;
std::string text = "Connected to: ";
int ticks = 0;
std::string windowName;
sf::Socket::Status st;
sf::Color color;
std::string mensaje;
float timeToDraw = 20.0f;
bool done = false;

//PAINTING + TURN SYSTEM
enum Mode { DRAWING, WAITING, ANSWERING, WAITINGANSWERS, NOTHING };
Mode actualMode = Mode::NOTHING;
bool drawing;
bool doneDrawing;
std::vector<Circle> circles;
sf::Image screenshotImage;
sf::Texture screenshotTexture;
sf::Texture* textPTR;
sf::Sprite screenshotSprite;
int radius = 5;
sf::Color circleColor = sf::Color::White;
sftools::Chronometer chrono;
bool firstTimeScreenshot = true;

std::string myName = "";

sf::Uint8 *pixels;

//sf::RenderWindow* drawingWindowPtr;
//sf::RenderWindow* chatWindowPtr;
bool recievedEnd = false;

bool nameEntered = false;
bool nameReply = false;
bool lobbySelected = false;
bool lobbyReply = false;
enum commands { NOM, DEN, CON, INF, MSG, IMG, WRD, GUD, BAD, WNU, WIN, DIS, END, RNK, RDY, TIM, CRE, COK, CNO, LIS, JOI, JOK, JNO};

std::vector<Lobby> lobbies = std::vector<Lobby>();
std::vector<std::string> playerNames;

Mode SetGetMode(int setOrGet, Mode mode) {
	std::lock_guard<std::mutex> guard(myMutex);
	//static Mode sMode = Mode::NOTHING;
	if (setOrGet == SET) {
		switch (mode)
		{
		case DRAWING:
			if (actualMode != DRAWING) { chrono.reset(true); doneDrawing = false; drawing = false; } // FIRST TIME CHANGING MODE | Setting drawing to false here, if user is still pressing lClick when time runs out we put it to false automaticly.
			break;
		case WAITING:
			if (actualMode != WAITING) { chrono.reset(false); drawing = false; } // FIRST TIME CHANGING MODE
			break;
		case ANSWERING:
			if (actualMode != ANSWERING) { chrono.reset(false); drawing = false; } // FIRST TIME CHANGING MODE
			break;
		case WAITINGANSWERS:
			if (actualMode != WAITINGANSWERS) { chrono.reset(true); drawing = false; std::vector<Circle>().swap(circles); } // FIRST TIME CHANGING MODE
			break;
		default:
			break;
		}
		actualMode = mode;
		
	}
	return actualMode;
}

void addMessage(std::string s) {
	std::lock_guard<std::mutex> guard(myMutex);
	aMensajes.push_back(s);
	if (aMensajes.size() > 25) {
		aMensajes.erase(aMensajes.begin(), aMensajes.begin() + 1);
	}
}

void PrintLobby() {
	std::cout << "entered printlobby" << std::endl;
	int i = 0;
	for each (Lobby l in lobbies) {
		if(l.numPlayers < l.maxPlayers) std::cout << i << ": " << l.name << " " << l.numPlayers << "/" << l.maxPlayers << " PW:" << l.pw << std::endl;
		i++;
	}
	std::cout << "exit printlobby" << std::endl;
}

void FiltrarLobbiesPorNombre(std::string nameToSearch) {
	//CLR consola
	system("cls");
	for (int i = 0; i < lobbies.size(); i++) {
		if (strcmp(nameToSearch.c_str(), lobbies[i].name.c_str()) == 0) {
			std::cout << i << ": " << lobbies[i].name << " " << lobbies[i].numPlayers << "/" << lobbies[i].maxPlayers << " PW:" << lobbies[i].pw << std::endl;
		}
	}
}

void PrintPlayerNames() {
	system("cls");
	std::cout << "Players actuales: ";
	for (int i = 0; i < playerNames.size(); i++) {
		std::cout << playerNames[i];
		if (i != playerNames.size() - 1) std::cout << ", ";
	}
	std::cout << std::endl;
}
void receiveFunction(sf::TcpSocket* socket, bool* _connected) {
	char receiveBuffer[2000];
	std::size_t _received;
	while (*_connected) {
		sf::Packet packet;
		sf::Socket::Status rSt = socket->receive(packet);
		if (rSt == sf::Socket::Status::Done/*_received > 0*/) {

			std::string str;
			std::string str2;
			int integer;
			int command;
			int pixelsSize;
			if (packet >> command) {
				switch (command) {
				case commands::DEN:
					std::cout << "The name is already in use" << std::endl;
					nameReply = true;
					break;
				case commands::CON: 
					std::cout << "Your name has been saved" << std::endl;
					nameEntered = true;
					nameReply = true;
					playerNames.push_back(myName);
					break;
				case commands::RNK: 
					//para recibir le ranking
					break;
				case commands::INF:
					//recibimos nombre y printamos que se ha conectado un user nuevo
					packet >> str; 
					playerNames.push_back(str);
					addMessage("EL USUARIO: '" + str + "' SE HA UNIDO A LA PARTIDA, (USA EL COMANDO READY PARA EMPEZAR)");
					PrintPlayerNames();
					break;
				case commands::MSG:
					std::cout << "MESSAGE RECIEVED" << std::endl;
					packet >> str >> str2;
					addMessage(str + str2);

					break;
				case commands::IMG:
					//recibir la imagen para printarla en window
					//PROCESS IMAGE:
					firstTimeScreenshot = true;
					std::cout << "IMAGE RECEIVED" << std::endl;
					int imgWidth, imgHeight;
					packet >> imgWidth;
					std::cout << "WIDTH" << imgWidth << std::endl;
					packet >> imgHeight;
					std::cout << "HEIGHT" << imgHeight << std::endl;
					pixelsSize = imgWidth * imgHeight * 4;
					for (int i = 0; i < pixelsSize; i++) {
						int tempint;
						sf::Uint8 tempUint;
						packet >> tempUint;
						pixels[i] = tempUint;
						//std::cout << int(pixels[i]) << ", " << std::endl;
					}
					std::cout << "IMAGE PASSED TO ARRAY" << std::endl;
					//CREATE IMAGE THEN TEXTURE THEN SPRITE
					screenshotImage.create(imgWidth, imgHeight, pixels);
					std::cout << "IMAGE CREATED FROM ARRAY" << std::endl;
					//tempTexture.create(imgWidth, imgHeight);
					//std::cout << "TEXTURE SETTUP" << std::endl;
					//screenshotTexture.loadFromImage(screenshotImage);
					//std::cout << "TEXTURE COPIED FROM IMAGE" << std::endl;
					//screenshotSprite.setTexture(screenshotTexture, true);
					//std::cout << "SPRITE CREATED FROM TEXTURE" << std::endl;
					//screenshotSprite.setPosition(0, 0);

					SetGetMode(0, Mode::ANSWERING);
					addMessage("COMIENZA EL TIEMPO DE ADIVINAR");
					break;
				case commands::WRD:
					packet >> str;
					addMessage("TE TOCA DIBUJAR");
					addMessage("LA PALABRA QUE DEBES DIBUJAR ES: " + str);
					SetGetMode(0, Mode::DRAWING);
					break;
				case commands::WNU:
					packet >> str;
					packet >> integer;
					addMessage("EL USUARIO '" + str + "' VA A DIBUJAR");
					addMessage("LA PALABRA CONTIENE " + std::to_string(integer) + " LETRAS");
					SetGetMode(0, Mode::WAITING);
					break;
				case commands::BAD:
					addMessage("LA PALABRA QUE HAS INTRODUCIDO ES INCORRECTA");
					break;
				case commands::GUD:
					addMessage("LA PALABRA QUE HAS INTRODUCIDO ES CORRECTA");
					break;
				case commands::WIN:
					packet >> str;
					packet >> integer;

					addMessage("EL USUARIO '" + str + "' HA ACERTADO. TIENE " + std::to_string(integer) + " PUNTOS");
					//actualizar scoreboard local, actualizando la puntuacion del jugador que ha acertado
					break;
				case commands::DIS: {
					packet >> str;
					addMessage("EL USUARIO: '" + str + "' SE HA DESCONECTADO");
					int nIndex = 0;
					bool tempFound = false;
					while (!tempFound && nIndex < playerNames.size()) {
						if (strcmp(str.c_str(), playerNames[nIndex].c_str()) == 0) {
							playerNames.erase(playerNames.begin() + nIndex);
							tempFound = true;
						}
						nIndex++;
					}
					PrintPlayerNames();
					//done = true;
					break;
				}
				case commands::TIM:
					addMessage("SE HA ACABADO EL TIEMPO DE ADIVINAR");
					SetGetMode(SET, Mode::NOTHING);
					break;
				case commands::END:
					std::vector<Lobby>().swap(lobbies);	//RESETEAMOS EL VECTOR DE LOBBIES
					std::vector<std::string>().swap(playerNames); //RESETEAMOS EL VECTOR DEL NOMBRE DE JUGADORES
					playerNames.push_back(myName);
					//mensaje indicando el ganador de la partida, indicando su nombre
					packet >> str;
					addMessage("EL USUARIO '" + str + "' HA GANADO LA PARTIDA. GG");
					std::cout << "EL USUARIO '" << str << "' HA GANADO LA PARTIDA. GG" << std::endl;
					//desconectar
					//done = true;
					lobbyReply = false;
					lobbySelected = false;
					recievedEnd = true;
					SetGetMode(SET, Mode::NOTHING);
					break;
				case commands::LIS: {
					std::cout << "received LIS" << std::endl;
					int numLobbies = 0;
					std::string lobbyName = "";
					int lobbyID = 0;
					bool pwNeeded = false;
					int maxPlayers = 0;
					int actualPlayers = 0;
					packet >> numLobbies;
					std::cout << numLobbies << std::endl;
					for (int i = 0; i < numLobbies; i++) {
						packet >> lobbyName;
						packet >> lobbyID;
						packet >> pwNeeded;
						packet >> maxPlayers;
						packet >> actualPlayers;
						Lobby tempLobby;
						tempLobby.lobbyId = lobbyID;
						tempLobby.maxPlayers = maxPlayers;
						tempLobby.name = lobbyName;
						tempLobby.numPlayers = actualPlayers;
						tempLobby.pw = pwNeeded;
						lobbies.push_back(tempLobby);
					}
					lobbyReply = true;

					break;
				}
				case commands::JOK: {
					lobbySelected = true;
					lobbyReply = true;
					
					int curPlayers;
					packet >> curPlayers;
 					for (int i = 0; i < curPlayers; i++) {
						std::string tempPlayerName;
						packet >> tempPlayerName;
						playerNames.push_back(tempPlayerName);
					}
					PrintPlayerNames();
					break;
				}
				case commands::JNO: {
					lobbyReply = true;
					break;
				}
				case commands::COK: {
					std::cout << "received COK" << std::endl;
					lobbySelected = true;
					lobbyReply = true;

					break;
				}
				case commands::CNO: {
					lobbyReply = true;
					break;
				}

				}	//</Switch>
			}
		}
	}
}

void blockeComunication() {

	receiveThread = std::thread(receiveFunction, &socket, &connected);

	while (!done && (st == sf::Socket::Status::Done) && connected)
	{

		//name enter phase
		while (!nameEntered) {
			//hacer que el usuario escriba el nombre
			std::string namePlayer;
			std::cout << "Please enter your name: ";
			std::cin >> namePlayer;
			myName = namePlayer;
			//enviar nombre
			sf::Packet newP;
			newP << commands::NOM << namePlayer;
			socket.send(newP);

			nameReply = false;
			while(!nameReply){} //espera a respuesta de server para cambiar este bool
		}

		while (!lobbyReply) {} //espera a respuesta de server LIS para cambiar este bool

		while (!lobbySelected) {	//lobbySelected = true en el recieve del JKO
			//PRINTLOBBY
			char filter = 'n';
			char joinMode = 'j';

			bool correct = false;
			while (!correct) {
				std::cin.clear();
				std::cin.ignore(10000, '\n');
				std::cout << "Want to join (j) or create (c) a lobby? ";
				std::cin >> joinMode;
				if ((joinMode == 'j' || joinMode == 'J') || (joinMode == 'c' || joinMode == 'C')) {
					correct = true;
				} else { std::cout << "INCORRECT INPUT. PLEASE ENTER A VALID OPTION! Try again..." << std::endl; }
			}

			if (joinMode == 'j' || joinMode == 'J') {
				PrintLobby();

				//Filtro
				std::cout << "Press 'y' to filter: ";
				std::cin >> filter;
				if (filter == 'y' || filter == 'Y') {
					std::cout << "Type the name of the server you want to find: ";
					std::string nameToSearch = "";
					std::cin >> nameToSearch;
					FiltrarLobbiesPorNombre(nameToSearch);
				}

				//cin para idLobby
				int desiredLobby = -1;
				std::string pass = "";
				correct = false;
				while (!correct) {
					std::cin.clear();
					std::cin.ignore(10000, '\n');
					std::cout << "Please type the number of the lobby you want to join: ";
					std::cin >> desiredLobby;
					if (desiredLobby < lobbies.size() && desiredLobby >= 0) {
						correct = true;
					} else { std::cout << "INCORRECT INPUT. PLEASE ENTER THE NUMBER OF THE LOBBY! Try again..." << std::endl; }
				}
				
				//if(pw) cin para pass si necesario
				if (lobbies[desiredLobby].pw) {
					std::cout << "Password Needed: ";
					std::cin >> pass;
				}

				sf::Packet joinLobbyPacket;
				joinLobbyPacket << commands::JOI;
				//joinLobbyPacket << myName;
				joinLobbyPacket << lobbies[desiredLobby].lobbyId;
				if (lobbies[desiredLobby].pw) { joinLobbyPacket << pass; }

				socket.send(joinLobbyPacket);
			}
			else if (joinMode == 'c' || joinMode == 'C') {	//CREATE LOBBY
				std::string desiredLobbyName = "";
				int desiredMaxPlayers = -1;
				int desiredMaxTurns = -1;
				char passW = 'n';
				bool passWanted = false;
				std::string desiredPassword = "1234";
				sf::Packet createLobbyPacket;
				createLobbyPacket << commands::CRE;

				//LOBBY NAME
				std::cout << "What name do you want for your lobby? (no spaces):" << std::endl;
				std::cin >> desiredLobbyName; //MIRAR SI ESTÁ PILLADO
				createLobbyPacket << desiredLobbyName;

				//LOBBY MAX PLAYERS
				correct = false;
				while (!correct) {
					std::cin.clear();
					std::cin.ignore(10000, '\n');
					std::cout << "Max Players?" << std::endl;
					std::cin >> desiredMaxPlayers;
					if (desiredMaxPlayers > 0 && desiredMaxPlayers <= 4) {
						correct = true;
						std::cout << "You wrote: " << desiredMaxPlayers << std::endl;
					}
					else { std::cout << "WRONG INPUT. PLEASE ENTER A NUMBER [0, 4]. Try again..." << std::endl; desiredMaxPlayers = -1; }
				}
				createLobbyPacket << desiredMaxPlayers;
				
				//LOBBY TURNS PER PLAYER
				correct = false;
				while (!correct) {
					std::cin.clear();
					std::cin.ignore(10000, '\n');
					std::cout << "Number of turns per player?" << std::endl;
					std::cin >> desiredMaxTurns;
					if (desiredMaxTurns > 0 && desiredMaxTurns <= 4) {
						correct = true;
					} else { std::cout << "WRONG INPUT. PLEASE ENTER A NUMBER [0, 4]. Try again..." << std::endl; }
				}
				createLobbyPacket << desiredMaxTurns;



				std::cout << "Do you want to put a password? (y/n)" << std::endl;
				std::cin >> passW;
				if (passW == 'y' || passW == 'Y') {
					std::cout << "What Password? (no spaces)" << std::endl;
					std::cin >> desiredPassword;
					createLobbyPacket << true;
					createLobbyPacket << desiredPassword;
				}
				else {
					createLobbyPacket << false;
				}

				socket.send(createLobbyPacket);
			}
			lobbyReply = false;
			while (!lobbyReply) {} //espera a respuesta de server para cambiar este bool

		}

		sf::Vector2i screenDimensions(800, 600);

		sf::RenderWindow window;
		window.create(sf::VideoMode(screenDimensions.x, screenDimensions.y), windowName);
		//chatWindowPtr = &window;

		//DRAWING WINDOW:
		sf::RenderWindow drawingWindow;
		drawingWindow.create(sf::VideoMode(screenDimensions.x, screenDimensions.y), "Pictionary Drawing Test", sf::Style::Titlebar);
		drawingWindow.setFramerateLimit(0);
		//drawingWindowPtr = &drawingWindow;

		//Creating texture with window size.
		sf::Vector2u windowSize = drawingWindow.getSize();
		textPTR = new sf::Texture();
		textPTR->create(windowSize.x, windowSize.y);

		pixels = new sf::Uint8[windowSize.x * windowSize.y * 4]; //width * height * 4 -> each pixel = 4 color channel of 8Bit unsigned int: R G B A

		sf::Font font;
		if (!font.loadFromFile("courbd.ttf"))
		{
			std::cout << "Can't load the font file" << std::endl;
		}

		std::string mensaje = "";

		sf::Text chattingText(mensaje, font, 14);

		chattingText.setFillColor(color);
		chattingText.setStyle(sf::Text::Regular);


		sf::Text text(mensaje, font, 14);
		text.setFillColor(color);
		text.setStyle(sf::Text::Regular);
		text.setPosition(0, 560);

		sf::RectangleShape separator(sf::Vector2f(800, 5));
		separator.setFillColor(sf::Color(200, 200, 200, 255));
		separator.setPosition(0, 550);
		
		//window is open
		while (window.isOpen() && drawingWindow.isOpen())
		{
			sf::Time time = chrono;
			sf::Event evento;
			while (window.pollEvent(evento))
			{
				std::string exitMessage;
				sf::Packet packet;
				switch (evento.type)
				{
				case sf::Event::Closed:
					//DISCONECT FROM SERVER
					done = true;
					std::cout << "CLOSE" << std::endl;
					connected = false;
					exitMessage = " >exit";
					packet << commands::MSG << exitMessage;
					socket.send(packet);
					window.close();
					drawingWindow.close();
					break;
				case sf::Event::KeyPressed:
					if (evento.key.code == sf::Keyboard::Escape) {
						window.close();
						drawingWindow.close();
					}
					else if (evento.key.code == sf::Keyboard::Return) //envia mensaje
					{
						sf::Packet packet;
						packet << commands::MSG << (" >" + mensaje).c_str();
						sf::Socket::Status tempSt = socket.send(packet);
						//addMessage(mensaje);
						if (strcmp(mensaje.c_str(), "exit") == 0) {
							std::cout << "EXIT" << std::endl;
							//addMessage("YOU DISCONNECTED FROM CHAT");
							connected = false;
							done = true;
							window.close();
							drawingWindow.close();
						}
						else if (strcmp(mensaje.c_str(), "ready") == 0) {
							sf::Packet newPacket;
							newPacket << commands::RDY;
							socket.send(newPacket);
						}
						mensaje = "";
					}
					break;
				case sf::Event::TextEntered:
					if (evento.text.unicode >= 32 && evento.text.unicode <= 126)
						mensaje += (char)evento.text.unicode;
					else if (evento.text.unicode == 8 && mensaje.length() > 0)
						mensaje.erase(mensaje.length() - 1, mensaje.length());
					break;
				}
			}

			//DRAWING EVENTS:
			sf::Event evnt;
			while (drawingWindow.pollEvent(evnt) && SetGetMode(1, NOTHING) == Mode::DRAWING) {	//If the actual mode is DRAWING we can draw
				switch (evnt.type)
				{
				case sf::Event::MouseButtonPressed:
					//Start drawing
					drawing = true;
					break;
				case sf::Event::MouseButtonReleased:
					//Stop drawing
					drawing = false;
					break;
				default:
					break;
				}
			}
			
			//Drawing System: ONLY IF MODE == DRAWING | drawing only is set to true if the mode is Drawing.
			if (drawing && int(time.asSeconds()) < timeToDraw) {
				std::cout << "TIME: " << time.asMilliseconds() << std::endl;
				circles.push_back(Circle(radius, sf::Color::Black, sf::Mouse::getPosition(drawingWindow)));
			}
			else if (int(time.asSeconds()) >= timeToDraw && !doneDrawing && SetGetMode(1, Mode::NOTHING) == Mode::DRAWING) {
				//STOP CHRONO + SAVE IMAGE:
				chrono.pause();
				doneDrawing = true;
				sf::Packet imagePacket;
				imagePacket << commands::IMG;
				textPTR = new sf::Texture();
				textPTR->create(windowSize.x, windowSize.y);
				textPTR->update(drawingWindow);
				screenshotImage = textPTR->copyToImage();
				int pixelsSize = windowSize.x * windowSize.y * 4;
				imagePacket << windowSize.x << windowSize.y;
				int counter = 0;
				for (int i = 0; i < pixelsSize; i++) {
					counter++;
					sf::Uint8 _tempUint;
					_tempUint = screenshotImage.getPixelsPtr()[i];
					pixels[i] = _tempUint;
					imagePacket << pixels[i];
				}
				std::cout << "MY PIXELS SIZE: " << counter << " | Image Size: " << screenshotImage.getSize().x * screenshotImage.getSize().y * 4 << std::endl;
				screenshotSprite.setTexture(*textPTR, true);
				screenshotSprite.setPosition(0, 0);
				//SEND IMAGE
				std::cout << "IMAGE SENT AFTER DRAWING" << std::endl;
				socket.send(imagePacket);
				SetGetMode(0, Mode::WAITINGANSWERS);
				addMessage("SE HA ACABADO EL TIEMPO DE DIBUJAR");
				std::cout << "set waiting answers";
			}
			else if (doneDrawing && SetGetMode(GET, Mode::NOTHING) == Mode::WAITINGANSWERS && int(time.asSeconds()) >= timeToDraw) {
				//ENVIAR TIME UP CON COMANDO TIM.
				std::cout << "turn done";
				sf::Packet newPacket;
				newPacket << commands::TIM;
				socket.send(newPacket);
				chrono.reset(false);
				chrono.pause();
			}
			
	//Draw -------------------------------------------------------------------------------------------------------------------------------
			window.draw(separator);
			for (size_t i = 0; i < aMensajes.size(); i++)
			{
				std::string chatting = aMensajes[i];
				chattingText.setPosition(sf::Vector2f(0, 20 * i));
				chattingText.setString(chatting);
				window.draw(chattingText);
			}
			std::string mensaje_ = mensaje + "_";
			text.setString(mensaje_);
			window.draw(text);

			window.display();
			window.clear();

			drawingWindow.clear();
			if (SetGetMode(1, Mode::NOTHING) == Mode::DRAWING) {
				if (circles.size() > 0) {
					for (int i = 0; i < circles.size() - 1; i++) {
						circles[i].draw(&drawingWindow);
					}
				}
			}
			else if (SetGetMode(1, Mode::NOTHING) == Mode::ANSWERING || SetGetMode(1, Mode::NOTHING) == Mode::WAITINGANSWERS) {
				if (firstTimeScreenshot && SetGetMode(1, Mode::NOTHING) == Mode::ANSWERING) {
					textPTR = new sf::Texture();
					textPTR->create(windowSize.x, windowSize.y);
					std::cout << "TEXTURE SETTUP" << std::endl;
					textPTR->loadFromImage(screenshotImage);
					std::cout << "TEXTURE COPIED FROM IMAGE" << std::endl;
					screenshotSprite.setTexture(*textPTR, true);
					std::cout << "SPRITE CREATED FROM TEXTURE" << std::endl;
					screenshotSprite.setPosition(0, 0);
					firstTimeScreenshot = false;
				}
				drawingWindow.draw(screenshotSprite);
			}
			drawingWindow.display();

			if (recievedEnd) {
				std::cout << "CLOSE WINDOWS" << std::endl;
				window.close();
				drawingWindow.close();
				recievedEnd = false;
			}
		}
	}
	receiveThread.join();
}

void main() {
	srand(time(NULL));
	color = sf::Color(rand() % 255 + 0, rand() % 255 + 0, rand() % 255 + 0, 255);
	st = sf::Socket::Status::Disconnected;
	bool serv;
	std::string serverMode;

	drawing = false;
	doneDrawing = false;
	circles = std::vector<Circle>();
	
	chrono.resume();

	std::cout << "Enter (c) for Client: ";
	std::cin >> connectionType;

	if (connectionType == 'c')
	{
		serv = false;
		do {
			ticks++;
			st = socket.connect(ip, 50000, sf::seconds(5.f));
			if (st != sf::Socket::Status::Done) std::cout << "NO SE PUDO CONECTAR PENDEJO TRAS 5s" << std::endl;
		} while (st != sf::Socket::Status::Done && ticks < 3);

		text += "Client";
		mode = 'r';
		windowName = "Client Chat Window";

	}

	if (st == sf::Socket::Status::Done) {
		connected = true;
		blockeComunication();
	}

	socket.disconnect();
}