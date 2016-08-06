

# How to run

```bash
curdir=$(pwd)
cd ../../..
source sourceme.sh
cd ${AIO_LIB_DIR} && make 
cd ${AIO_LIB_DIR}/wrappers/java && make -f GNUMakefile inplace_java
cd ${curdir}
make jar
java -jar *.jar
```
