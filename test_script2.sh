netid=oxo150430 # change your netid here
PREFIX=cs6378/proj3 # change here to your directory of choice

n_servers=3
n_clients=5

n_requests=5 # change here, maximum 10
alg_type=MKW # RA/MKW
gen_random_input=1 # 0 if not gen random input, otherwise 1

ssh ${netid}@dc04.utdallas.edu "rm -rf $PREFIX/test_data/servers"
ssh ${netid}@dc04.utdallas.edu "cp -R  $PREFIX/test_data_init/servers $PREFIX/test_data"
ssh ${netid}@dc04.utdallas.edu "rm -rf $PREFIX/logs/*"

for (( i=1; i<=$n_servers; i++ ))
do
    let "id=i-1"
    ssh ${netid}@dc01.utdallas.edu "./$PREFIX/build/run_server $id $PREFIX/config_local/config_servers.txt $n_servers $n_clients $PREFIX/logs $PREFIX/test_data/servers" &
done

for (( i=1; i<=$n_clients; i++ ))
do
    let "id=i-1"
    ssh ${netid}@dc01.utdallas.edu "./$PREFIX/build/run_client $id $PREFIX/config_local/config_servers.txt $PREFIX/config_local/config_clients.txt $n_servers $n_clients $PREFIX/logs $PREFIX/test_data/clients $alg_type $n_requests $gen_random_input" &
done

wait
echo "All done"
