#include "client.h"


int main() {
	Client client;
	client.InitializeConnection();
	client.TryToConnect();
	client.WaitingForCommands();
	client.StopConnection();
	return 0;
}