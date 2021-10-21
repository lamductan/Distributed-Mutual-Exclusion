rm -rf test_data/servers
cp -R  test_data_init/servers test_data
cp -R  test_data_init/clients test_data
rm -rf logs/server.txt
rm -rf logs/client.txt

n_servers=3
n_clients=5

n_requests=5 # change here, maximum 10
alg_type=MKW # RA/MKW
gen_random_input=1 # 0 if not gen random input, otherwise 1

for (( i=1; i<=$n_servers; i++))
do
    let "id=i-1"
	./build/run_server $id config_local/config_servers.txt $n_servers $n_clients logs test_data/servers &
done

for (( i=1; i<=$n_clients; i++))
do
    let "id=i-1"
	./build/run_client $id config_local/config_servers.txt config_local/config_clients.txt $n_servers $n_clients logs test_data/clients $alg_type $n_requests $gen_random_input &
done

wait
echo "All done"
