cd ./src
make clean
make web
cd ../cluster
./stop_cluster.sh
sleep 2
./start_cluster.sh

