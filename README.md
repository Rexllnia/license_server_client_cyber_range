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
输入错误的的激活码 123 假设1111:2222:2222:2222:2222:2222:2222:2222是你的IPV6地址  
./test1 123 1111:2222:2222:2222:2222:2222:2222:2222  
输入正确的激活码 CODE_ABC123    
./test1 CODE_ABC123 1111:2222:2222:2222:2222:2222:2222:2222
激活后无需再次激活
./test1
