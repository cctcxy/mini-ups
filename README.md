# mini UPS
## To Run
First ensure the world and the amazon is running, then under `./docker-deploy/`

	sudo docker-compose up --build
To access the frontend, visit
	
	http://<host_address>:8000/ups/
### set address
The backend would read the `./docker-deploy/backend/config` file for world/amazon address; the first 4 lines are taken as world host, world port, amazon host, and amazon address accordingly. If the world/amazon server runs under the same host, you might want to use `host.docker.internal` as the host address.
### some macros
You might want to ensure in the file `./docker-deploy/backend/include/util.h`, some macros are set as follows: 

	#define  CONFIG_HOST  1
	#define  UNIT_TEST  0
	#define  SINGLE_FRONT_END_SOCKET  0

## World simulator
https://github.com/yunjingliu96/world_simulator_exec