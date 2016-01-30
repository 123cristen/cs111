#!/bin/sh
# test.sh
# CS111 Winter 2016
# Lab1 Cristen Anderson, Sunnie So

# exit with 0 upon success.
# exit with 1 when at least one test fails.

# tmp files
# exit if fails to create files
a=/tmp/a || exit 1
b=/tmp/b || exit 1
c=/tmp/c || exit 1
d=/tmp/d || exit 1
e=/tmp/e || exit 1
# give files content
echo "Here is a file" > "$a"
> "$b"
> "$c"
> "$d"
> "$e"

# Test 1: open invalid file
./simpsh --rdonly noFile 2>&1 | grep "Error: open returned unsuccessfully" > /dev/null
if [ $? -eq 0 ]
	then 
		echo "Test 1: success"	
	else
		echo "Test 1: failure: --rdonly should not open invalid filename"
		exit 1
fi

# Test 2: simple command
./simpsh --rdonly $a --wronly $b --wronly $c --command 0 1 2 cat -
cat $a > $c
cat $b > $d
diff -u $c $d > /dev/null
if [ $? -ne 0 ]
	then 
		echo "Test 2: success"	
	else
		echo "Test 2: failure: --command should execute simple command 'cat'"
		exit 1
fi
> "$b"
> "$c"
> "$d"

# Test 3: report invalid file descriptor
./simpsh --command 0 1 2 echo "hi" 2>&1 | grep "initiation" > /dev/null
if [ $? -ne 0 ]
	then 
		echo "Test 3: success"	
	else
		echo "Test 3: failure: --command should report uninitialized file descriptor"
		exit 1
fi

# Test 4: write to read only file
./simpsh --rdonly $a --rdonly $b --wronly $c --command 0 1 2 cat -
cat $c | grep "Bad file descriptor" > /dev/null
if [ $? -ne 0 ]
	then 
		echo "Test 4: success"	
	else
		echo "Test 4: failure: --command should not write to read only file"
		exit 1
fi
> "$b"
> "$c"

# Test 5: correct number/type of arguments
./simpsh --rdonly $a --wronly $b --wronly $c --command 0 1 cat - 2>&1 | grep "Error: Incorrect usage of --command. Requires integer argument." > /dev/null
if [ $? -ne 0 ]
	then 
		echo "Test 5: success"	
	else
		echo "Test 5: failure: --command should have correct number of arguments"
		exit 1
fi
> "$b"
> "$c"

# Test 6: verbose prints options
./simpsh --verbose --rdonly $a --wronly $b --wronly $c --command 0 1 2 cat - > $d
echo '--rdonly /tmp/a ' > $e; echo '--wronly /tmp/b ' >> $e; echo '--wronly /tmp/c ' >> $e; echo '--command 0 1 2 cat - ' >> $e
diff -u $d $e > /dev/null 
if [ $? -ne 0 ]
	then 
		echo "Test 6: success"	
	else
		echo "Test 6: failure: --verbose should print options"
		exit 1
fi
> "$b"
> "$c"
> "$d"
> "$e"

# Test 7: file flags correctly passed to open()
echo "'a' has content" > $a
./simpsh --trunc --rdwr $a
diff -u $a $b > /dev/null
if [ $? -ne 0 ]
	then 
		echo "Test 7: success"	
	else
		echo "Test 7: failure: --trunc should be passed to open()"
		exit 1
fi

echo "All tests succeeded"

# Delete temp files
rm "$a"
rm "$b"
rm "$c"
rm "$d"
rm "$e"

exit 0

########################################## ##################### ##################### 
#
#    Kim's tests for the flags
#
###################### ##################### ##################### ##################### 

#for append
#emacs a (write gibberish)
# ./simpsh --rdonly a --append --wronly b --wronly c  --command 0 1 2 cat - - --command 0 1 2 cat - - 

#for directory
# emacs a (gibberish) 
# ./simpsh --directory --rdonly a --creat --trunc --wronly b --creat --wronly c --command 0 1 2 cat - - 


# test for pipe: create a, b, c in advance
# --rdonly a --wronly b --wronly c --pipe --command 0 4 2 cat - - --command 3 1 2 cat - -


