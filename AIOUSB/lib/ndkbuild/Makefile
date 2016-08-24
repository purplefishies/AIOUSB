

all: builtndk.txt


builtndk.txt:
	ndk-build
	touch builtndk.txt

clean:
	ndk-build clean
	rm -rf obj
	rm -rf libs
