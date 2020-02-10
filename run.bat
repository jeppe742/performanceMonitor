@echo off

del *.exe *.txt 2>NUL

echo "Compiling files.."
gcc test.c -o test_c.exe
g++ test.cpp -o test_cpp.exe


echo "runnint test 1"
echo "Start io" %time% >>timings.txt
for /l %%x in (1,1,50000) do (
echo %%x>>log.txt
)
echo "Stop io" %time% >>timings.txt

echo "runnint test 2"
echo "Start io separate" %time% >>timings.txt
for /l %%x in (1,1,50000) do (
echo %%x>>logs\%%x.txt
)
echo "Stop io separate" %time% >>timings.txt

echo "running test 3"
echo "Start test_c no io" %time% >> timings.txt
for /l %%x in (1,1,50000) do (
test_c.exe 
)
echo "Stop test_c no io" %time% >> timings.txt

echo "running test 4"
echo "Start test_cpp no io" %time% >> timings.txt
for /l %%x in (1,1,50000) do (
test_cpp.exe
)
echo "Stop test_cpp no io" %time% >> timings.txt

echo "recompiling.."
gcc test.c -o test_c.exe -DUSE_IO
g++ test.cpp -o test_cpp.exe -DUSE_IO

echo "running test 5"
echo "Start test_c print" %time% >> timings.txt
for /l %%x in (1,1,50000) do (
test_c.exe 
)
echo "Stop test_c print" %time% >> timings.txt

echo "running test 6"
echo "Start test_cpp print" %time% >> timings.txt
for /l %%x in (1,1,50000) do (
test_cpp.exe 
)
echo "Stop test_cpp print" %time% >> timings.txt

echo "running test 7"
echo "Start test_c io" %time% >> timings.txt
for /l %%x in (1,1,50000) do (
test_c.exe >> test_c.txt
)
echo "Stop test_c io" %time% >> timings.txt

echo "running test 8"
echo "Start test_cpp io" %time% >> timings.txt
for /l %%x in (1,1,50000) do (
test_cpp.exe >> test_cpp.txt
)
echo "Stop test_cpp io" %time% >> timings.txt


gcc main.c -o main_c.exe -lpsapi -DTlHelp32
g++ main.cpp -o main_cpp.exe -lpsapi -DTlHelp32

echo "running test 9"
echo "Start main_c TlHelp32" %time% >> timings.txt
for /l %%x in (1,1,50000) do (
main_c.exe >> main_c.txt
)
echo "Stop main_c TlHelp32" %time% >> timings.txt

echo "running test 10"
echo "Start main_cpp TlHelp32 " %time% >> timings.txt
for /l %%x in (1,1,50000) do (
main_cpp.exe >> main_cpp.txt
)
echo "Stop main_cpp TlHelp32" %time% >> timings.txt

gcc main.c -o main_c.exe -lpsapi 
g++ main.cpp -o main_cpp.exe -lpsapi 

echo "running test 11"
echo "Start main_c NTquery" %time% >> timings.txt
for /l %%x in (1,1,50000) do (
main_c.exe >> main_c.txt
)
echo "Stop main_c NTquery" %time% >> timings.txt

echo "running test 12"
echo "Start main_cpp NTquery" %time% >> timings.txt
for /l %%x in (1,1,50000) do (
main_cpp.exe >> main_cpp.txt
)
echo "Stop main_cpp NTquery" %time% >> timings.txt


