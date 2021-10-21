netid=tdl190003 # change your netid here
PREFIX=dev/cs6378/projects/proj3/cpp/distributed-me # change here to your directory of choice

n_servers=3
n_clients=5

n_requests=5 # change here, maximum 10
alg_type=MKW # RA/MKW
gen_random_input=1 # 0 if not gen random input, otherwise 1

ssh ${netid}@dc04.utdallas.edu "rm -rf $PREFIX/test_data/servers"
ssh ${netid}@dc04.utdallas.edu "cp -R  $PREFIX/test_data_init/servers $PREFIX/test_data"
ssh ${netid}@dc04.utdallas.edu "rm -rf $PREFIX/logs/client.txt"
ssh ${netid}@dc04.utdallas.edu "rm -rf $PREFIX/logs/server.txt"

for (( i=1; i<=$n_servers; i++ ))
do
    let "id=i-1"
    ssh ${netid}@dc0$i.utdallas.edu "./$PREFIX/build/run_server $id $PREFIX/config/config_servers.txt $n_servers $n_clients $PREFIX/logs $PREFIX/test_data/servers" &
done

for (( i=1; i<=$n_clients; i++ ))
do
    let "id=i-1"
    ssh ${netid}@dc1$i.utdallas.edu "./$PREFIX/build/run_client $id $PREFIX/config/config_servers.txt $PREFIX/config/config_clients.txt $n_servers $n_clients $PREFIX/logs $PREFIX/test_data/clients $alg_type $n_requests $gen_random_input" &
done

wait
echo "All done"
