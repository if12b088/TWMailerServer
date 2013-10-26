/* myserver.c */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <string>
#include <string.h>
//#include <thread>
#include "File.h"
#include "MessageService.h"
#include "Helper.h"

#define BUF 1024

void printUsage(std::string programName) {
	std::cerr << "Usage:" << programName << " PORT(>1023) DIRPATH" << std::endl;
	exit(EXIT_FAILURE);
}

std::string removeNewline(std::string s) {
	if (!s.empty() && s[s.length() - 1] == '\n') {
		s.erase(s.length() - 1);
	}
	return s;
}

int main(int argc, char *argv[]) {

	// Ausgabe der Parameter
	std::cout << "argv0: " << argv[0] << std::endl;
	std::cout << "argv1: " << argv[1] << std::endl;
	std::cout << "argv2: " << argv[2] << std::endl;
	std::cout << "argc: " << argc << std::endl;

	if (argc != 3) {
		printUsage(argv[0]);
	}
	short port = (short) atoi(argv[1]);

	if (port < 1024) {
		printUsage(argv[0]);
	}
	std::string dirPath = argv[2];

	MessageDao* dao = new MessageDao(dirPath.c_str());
	MessageService* service = new MessageService(dao);

	struct sockaddr_in address, cliaddress;

	int create_socket;

	if ((create_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Socket error");
		return EXIT_FAILURE;
	}

	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	// damit man den gleichen Port nochmal oeffnen kann
	int yes = 1;
	if (setsockopt(create_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
			== -1) {
		perror("reuse error");
		return EXIT_FAILURE;
	}

	if (bind(create_socket, (struct sockaddr *) &address, sizeof(address))
			!= 0) {
		perror("bind error");
		return EXIT_FAILURE;
	}
	listen(create_socket, 5);

	socklen_t addrlen = sizeof(struct sockaddr_in);

	char buffer[BUF];
	char returnBuffer[BUF];
	std::string returnMsg;
	int size;
	//std::vector<std::thread> threads;

	while (1) {
		printf("Waiting for connections...\n");
		int new_socket = accept(create_socket, (struct sockaddr *) &cliaddress,
				&addrlen);
		if (new_socket > 0) {
			printf("Client connected from %s:%d...\n",
					inet_ntoa(cliaddress.sin_addr), ntohs(cliaddress.sin_port));
			strcpy(buffer, "Welcome to myserver, Please enter your command:\n");
			send(new_socket, buffer, strlen(buffer), 0);
		}
		do {

			size = Helper::readline(new_socket, buffer, BUF - 1);

			if (size > 0) {
				buffer[size] = '\0';
				//returnBuffer[0] = '\0';
				returnMsg = "";
				//SEND
				if (strcmp(buffer, "SEND\n") == 0) {
					char fromChar[BUF];
					std::string from;
					char toChar[BUF];
					std::string to;
					char subjectChar[BUF];
					std::string subject;
					char textTempChar[BUF];
					std::string textTempStr;
					std::string text;

					int sizeFrom = Helper::readline(new_socket, fromChar,
					BUF - 1);
					from = removeNewline(std::string(fromChar));

					int sizeTo = Helper::readline(new_socket, toChar, BUF - 1);
					to = removeNewline(std::string(toChar));

					int sizeSubject = Helper::readline(new_socket, subjectChar,
					BUF - 1);
					subject = removeNewline(std::string(subjectChar));

					//int sizeText = Helper::readline(new_socket, text, 81);
					do {
						textTempStr = "";
						textTempChar[0] = '\0';
						int sizeText = Helper::readline(new_socket,
								textTempChar, BUF - 1);
						textTempStr = std::string(textTempChar);
						if (textTempStr != ".\n") {
							text.append(textTempStr);
						}
						std::cout << textTempStr << std::endl;

					} while (textTempStr != ".\n");
#ifdef _DEBUG
					std::cout << "From: " << from << ", size: " << from.length() << std::endl;
					std::cout << "To: " << to << ", size: " << to.length() << std::endl;
					std::cout << "Subject: " << subject << ", size: " << subject.length() << std::endl;
					std::cout << "Text: " << text << ", size: " << text.length() << std::endl;
#endif

					Message msg;
					msg.setFrom(from);
					msg.setTo(Helper::splitString(to, ";"));
					msg.setSubject(subject);
					msg.setText(text);

					//TODO Hier sollte auch noch das Attachment ausgelesen und hinzugefügt werden

					if (service->sendMsg(msg)) {
						returnMsg = "OK\n";
					} else {
						returnMsg = "ERR\n";
						//strcpy(returnBuffer, "ERR\n");
					}

				}
				//LIST
				if (strcmp(buffer, "LIST\n") == 0) {
					char userChar[BUF];
					std::string user;

					int sizeUser = Helper::readline(new_socket, userChar,
							BUF - 1);
					user = removeNewline(std::string(userChar));

#ifdef _DEBUG
					std::cout << "User: " << user << ", size: " << user.length() << std::endl;
#endif

					std::list<Message> msgList = service->listMsg(user);
					std::stringstream ss;

					ss << msgList.size() << "\n";

					for (std::list<Message>::iterator it = msgList.begin();
							it != msgList.end(); it++) {
						Message msg = *it;
						ss << msg.getMsgNr() << ": " << msg.getSubject()
								<< "\n";
					}

					//strcpy(returnBuffer, ss.str().c_str());
					returnMsg = ss.str();

				}
				//READ
				if (strcmp(buffer, "READ\n") == 0) {
					char userChar[BUF];
					std::string user;
					char nrChar[BUF];
					std::string nr;

					int sizeUser = Helper::readline(new_socket, userChar,
							BUF - 1);
					user = removeNewline(std::string(userChar));

					int sizeNr = Helper::readline(new_socket, nrChar,
							BUF - 1);
					//TODO wirklich in string umwandeln
					nr = removeNewline(std::string(nrChar));
					nrChar[strlen(nrChar) - 1] = '\0';

#ifdef _DEBUG
					std::cout << "User: " << user << ", size: " << user.length() << std::endl;
					std::cout << "Nr: " << nr << ", size: " << nr.length() << std::endl;
#endif

					Message msg = service->readMsg(user, atol(nrChar));

					std::stringstream ss;

					ss << "Nachricht mit der Nummer: " << msg.toString()
							<< "\n";

					//strcpy(returnBuffer, ss.str().c_str());

					returnMsg = ss.str();

				}
				//DEL
				if (strcmp(buffer, "DEL\n") == 0) {
					char userChar[BUF];
					std::string user;
					char nrChar[BUF];
					std::string nr;

					int sizeUser = Helper::readline(new_socket, userChar,
							BUF - 1);
					user = removeNewline(std::string(userChar));
					int sizeNr = Helper::readline(new_socket, nrChar,
							BUF - 1);
					//TODO wirklich in string umwandeln
					nr = removeNewline(std::string(nrChar));
					nrChar[strlen(nrChar) - 1] = '\0';

#ifdef _DEBUG
					std::cout << "User: " << user << ", size: " << user.length() << std::endl;
					std::cout << "Nr: " << nr << ", size: " << nr.length() << std::endl;
#endif

					//convert nr to long
					if (service->deleteMsg(user, atol(nrChar))) {
						returnMsg = "OK\n";
						//strcpy(returnBuffer, "OK\n");
					} else {
						returnMsg = "ERR\n";
						//strcpy(returnBuffer, "ERR\n");
					}
				}

				//printf("Message received: %s\n", buffer);
			} else if (size == 0) {
				printf("Client closed remote socket\n");
				break;
			} else {
				perror("recv error");
				return EXIT_FAILURE;
			}

			//answer

			if (send(new_socket, returnMsg.c_str(), returnMsg.length(), 0) == -1) {
				perror("Send error");
				return EXIT_FAILURE;
			}

//			if (send(new_socket, returnBuffer, strlen(returnBuffer), 0) == -1) {
//				perror("Send error");
//				return EXIT_FAILURE;
//			}
		} while (strncmp(buffer, "quit", 4) != 0);
		close(new_socket);
	}
	close(create_socket);
	return EXIT_SUCCESS;
}
