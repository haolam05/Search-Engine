# Bug 1

## A) How is your program acting differently than you expect it to?
- The unit test for ServerSocket failed, specifically server_addr and server_dns_name
were handled incorrectly.

## B) Brainstorm a few possible causes of the bug
- We reused a lot of code from the lecture, we might miss something when copying over.
- Failure to return server_addr and server_dns_name thru output parameters.
- IPv4 and IPv6 versions might not be handled correctly.

## C) How you fixed the bug and why the fix was necessary
- While checking if we copied everything correctly, we realized that we need to handle IPv4
and IPv6 separately (AF_INET and AF_INET6). After separating them into 2 different cases,
the issue was gone.


# Bug 2

## A) How is your program acting differently than you expect it to?
- There was memory leak when running valgrind the test for FileReader.

## B) Brainstorm a few possible causes of the bug
- There could be issues with our previous hw due since we are using our version.
- The test might call other functions that we have not implemented yet.
- Maybe we forgot to free resources in case of failure.

## C) How you fixed the bug and why the fix was necessary
- We looked back the function ReadFileToString() from the previous hw. When reading the spec
of the function in the header file, it mentioned that "The caller is responsible for freeing
the returned pointer." After adding the free statement for the returned pointer, there was no
memory leaks.


# Bug 3

## A) How is your program acting differently than you expect it to?
- The test for HttpConnection failed, and the failure was due to ParseRequest() function.

## B) Brainstorm a few possible causes of the bug
- We might used the wrong token to split.
- We did not use the boost library correctly since we looked up on StackOverflow.
- We might not handle all cases of the result after the split function (e.g. nothing to split).

## C) How you fixed the bug and why the fix was necessary
- After verifying that the token we used to split was correct, we continue to check if we used
the boost library correctly. We have a hard time trying to navigate the official document for the
boost library. We noticed that up the top we include <boost/algorithm/string.hpp>. Since used
boost::split instead of boost::algorithm::split, we knew where the problem was. After searching
online for how to use boost::algorithm::split, we fixed our code correspondingly, and it worked.
