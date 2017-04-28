SHERLOK is a java application monitor

To compile this project, you could create a VisualStudio project
In the project properties you add the JNI definitions from Java-JDK as additional include:

	C:\Program Files\Java\jdk1.8.0_66\include;C:\Program Files\Java\jdk1.8.0_66\include\win32

In the preprocessor statement you add the following parameter
	_WINDOWS

In the linker properties you add the following libraries in additional dependencies:
	Ws2_32.lib
    _WINSOCK_DEPRECATED_NO_WARNINGS
	
In the general tab 	
	Target name = sherlok
	Target type = shared library (.dll)


You start your Java application with following command line:
	java -agentlib:sherlok <your parameter> <your class>
	
You access the sherlok with the telnet console
	telnet localhost 2424
	user/password = Administrator/sherlok
	

Read the user specification of sherlok on how to continue.


