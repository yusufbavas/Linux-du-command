# Linux-du-command
System Programing Lecture

This project is based on the “ du “ shell utility. The du utility displays the sizes of the subdirectories of 
the tree rooted at the directory specified by its command-line argument.

du: Develop a program named “ buNeDu ” that uses a depth-first search strategy to display the sizes of the
subdirectories in a tree rooted at the specified starting path. 

du_fork:  This is based on the previous one. In this project, you will create a new process for each
directory to get the sizes. Each created processes will be responsible for:

	• Finding the size of the directory given by parent process (adding the sizes of subdirectories if -z
	  option is given),
	• Creating new processes to find the sizes of subdirectories,
	• Writing the PID of the process, size and path of the directory to a
	  single global file.

du_fifo:  In this project, you will create a new process for each directory to get the size of that directory. Each created processes will be responsible for:

	• Finding the size of the directory given by parent process,
	• Creating new processes to find the sizes of subdirectories. The created child processes must be able to run in parallel,
	• Writing the PID of the process, size in kilobytes and path of the directory to a single global FIFO. 
		As multiple processes will write the same FIFO at the same time. Remove the FIFO after execution.
	
	If -z option is given, the processes will also be responsible for:
		• Receiving the total size of each subdirectory from pipe connected to child processes,
		• Sending the total size of current directory to parent process via pipe