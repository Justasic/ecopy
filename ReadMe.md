Extreme Copy
===

This is (an attempt) at making the world's fastest copy command exclusively for Linux.

FAQ
===

#### Why did you make this command?
With the increase prevalence of faster and faster storage mediums like NVMe, traditional tools like rcopy, rsync, cp, etc. are not fast enough when copying data between drives. If you have ever needed to copy a home folder or maybe the entire linux install from one drive to another, you can understand how painful this is. Most of these tools were created with the intent to copy between a network location and a local storage location or on a storage location not fast enough (leading to lots of I/O wait).


Background
===

At my job I must move a lot of Microsoft Windows user profiles around between different computers, many users have a lot of stuff in their profiles and it takes a long time to copy the data (what I feel is unnecessarily long when it's a copy between two NVMe drives). I've decided to write this tool as a way to copy data even faster so the copy doesn't take as long. 

Internally I plan to use io\_uring, threads, and a recursive descent discovery algorithm for recursive directory copies. Ideally this will allow threads to discover files, dispatch their copy to the kernel via io_uring (which performs the copy operation), then move on and not having to care about whether the copy succeeded or not. Worker threads should be able to handle any pending jobs from the completion queue (such as longer copy operations).


License
===

GNU Lesser General Public License
