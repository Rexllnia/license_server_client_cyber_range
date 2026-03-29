# license_server_client_cyber_range
激活码服务器客户端靶场  

编译  
mkdir ./build  
cd ./build  
cmake ../  
make    

运行  
服务器运行  
./license_server  
输入错误的的激活码 123  
./test1 123  
输入正确的激活码 CODE_ABC123  
./test1 CODE_ABC123  
激活后无需再次激活
./test1