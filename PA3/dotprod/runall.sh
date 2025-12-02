touch reg_dotprod.c ur4_dotprod.c ae4_dotprod.c sse_dotprod.c
make SIZE=32768
./run.sh
touch reg_dotprod.c ur4_dotprod.c ae4_dotprod.c sse_dotprod.c
make SIZE=65536
./run.sh
touch reg_dotprod.c ur4_dotprod.c ae4_dotprod.c sse_dotprod.c
make SIZE=131072
./run.sh
touch reg_dotprod.c ur4_dotprod.c ae4_dotprod.c sse_dotprod.c
make SIZE=262144
./run.sh
