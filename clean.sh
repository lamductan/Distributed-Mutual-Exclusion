netid=tdl190003 # change your netid here
PREFIX=dev/cs6378/projects/proj3/cpp/distributed-me # change here to your directory of choice

n_servers=3
n_clients=5
alg_type=RA # MKW

n_requests=5 # change here, maximum 10

ssh ${netid}@dc01.utdallas.edu "rm -rf $PREFIX/test_data/servers"
ssh ${netid}@dc01.utdallas.edu "cp -R  $PREFIX/test_data_init/servers $PREFIX/test_data"
ssh ${netid}@dc01.utdallas.edu "rm -rf $PREFIX/logs/client.txt"
ssh ${netid}@dc01.utdallas.edu "rm -rf $PREFIX/logs/server.txt"

for (( i=1; i<=$n_servers; i++))
do
    ssh ${netid}@dc0$i.utdallas.edu "killall run_server" &
done

for (( i=1; i<=$n_clients; i++))
do
    ssh ${netid}@dc1$i.utdallas.edu "killall run_client" &
done

wait
echo "Done clean."