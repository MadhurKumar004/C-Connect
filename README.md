# C-Connect

A lightweight chat application written in C that lets you experience the joys of local networking! 

## Features
- Multi-client chat using select()
- Real-time messaging
- Join/Leave notifications
- Simple and clean interface

## Building
```bash
gcc -pthread -Wall -Wextra -Iinclude -o server src/server.c
gcc -pthread -Wall -Wextra -Iinclude -o client src/client.c
```

## Usage
1. Start the server:
```bash
./server
```

2. Connect with clients:
```bash
./client 127.0.0.1
```

## Network Connection Guide

To connect from another device (phone, another computer) on the same network:

1. Start the server and note the IP address displayed
2. On the client device, use that IP address:
```bash
./client <server_ip>
```

Common Issues:
1. Connection Refused? Check:
   - Server is running
   - Using correct IP address
   - No firewall blocking port 8080
   - Both devices are on the same network

Pro Tip: If 127.0.0.1 works but network IP doesn't, your firewall might be blocking connections. Try:
```bash
sudo ufw allow 8080    # Ubuntu/Debian
```
or
```bash
sudo firewall-cmd --add-port=8080/tcp --permanent    # Fedora/RHEL
sudo firewall-cmd --reload
```

## 🏠 Home Sweet Home (Network)

WARNING: This chat app is like a shy introvert - it only works on your local network! 
Don't try to use it to chat with your friend in another country... unless you both share the same WiFi router! 

Think of it as a "strictly-local" social network. Perfect for:
- Chatting with yourself (we don't judge!)
- Talking to your roommate without leaving your bed
- Pretending you're a hacker while actually just messaging localhost

Remember: What happens in 127.0.0.1, stays in 127.0.0.1 

## Tips
- Use `127.0.0.1` or `localhost` to connect to a server on the same machine
- For LAN chat, use the server's local IP address
- Maximum 100 clients (but let's be honest, you probably don't have that many friends on your local network)

## Commands
- Just type and press enter to send a message
- That's it! We keep things simple here!

## Contributing
Feel free to contribute! Just remember: this app is like a cat - it prefers to stay at home.
