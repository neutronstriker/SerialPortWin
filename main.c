#include <windows.h>
#include <stdio.h>

int main()
{
    //windows type variables
	HANDLE	hCom; //Handle variable
	DWORD	rwlen; // read/write length

    char send[32];
    char receive[32]= {0};//well i didn't test much as what is the maximum size of
    //windows serial receive buffer but i tested it upto 94 characters and worked well
    //but i am not sure that is this buffer size a feature of the prolific pl2303 driver
    //or a generic windows feature. However it would be safe to assume that it can
    //buffer 32 characters.

    int port,n;
    char port_name[20];

    printf("Please input serial port number ");
    scanf("%d",&port);

	/*	Open COM device	*/

	sprintf( port_name, "\\\\.\\COM%d", port );

	hCom = CreateFile( port_name, GENERIC_READ|GENERIC_WRITE,
					0, 0, OPEN_EXISTING, 0, 0 );
    //this is the CreateFile() which creates an instance of the SerialPort
    //accessible for read or write operations
    //the code in it is self-explanatory

        if( hCom==INVALID_HANDLE_VALUE )
        {
            printf( "\terror: COM%d is not available.\n", port );
            return -2;
        }

    /* optional but important part */

    //The DCB struct contains the parametes to confiure a serial port
    //if not used to configure then driver defaults will be used
    //if you change any port properties for example baud rate from
    //device manager it will not take effect immediately sometimes
    //you need to either restart the device or plug out and plug in the
    //device again for new parameters to take effect .
    //SO the below dcb method is more appropriate

    DCB dcbSerialParams = {0};

    dcbSerialParams.DCBlength=sizeof(dcbSerialParams);

    if (!GetCommState(hCom, &dcbSerialParams))
    {
        printf("Unable to get the state of serial port");
    //error getting state
    }

    dcbSerialParams.BaudRate=CBR_9600;
    dcbSerialParams.ByteSize=8;
    dcbSerialParams.StopBits=ONESTOPBIT;
    dcbSerialParams.Parity=NOPARITY;

    if(!SetCommState(hCom, &dcbSerialParams))
    {
        printf("Unable to set serial port settings\n");
               //error setting serial port state
    }

    /* DCB optional part ends here */

    /* COMTIMEOUTS Optional Part but very usefull especially against ReadHAngs */

    COMMTIMEOUTS timeouts={0};

    timeouts.ReadIntervalTimeout=50;
    timeouts.ReadTotalTimeoutConstant=50;
    timeouts.ReadTotalTimeoutMultiplier=10;
    timeouts.WriteTotalTimeoutConstant=50;
    timeouts.WriteTotalTimeoutMultiplier=10;

    if(!SetCommTimeouts(hCom, &timeouts))
    {

        printf("Error setting Serial Port timeouts property\n");
        //error occureed. Inform user
    }

        printf("COM%d opened successfully\n",port);



        printf("enter a string to send via serial port\n");

        //gets(send); because of some reason gets() is not working
        //that is why i am using scanf()
        scanf("%s",send);



        n = strlen(send); //number of bytes to transmit

		//Sleep(2000);//wait 2 secs, it is totally not necessary

        WriteFile( hCom, send, n, &rwlen, 0 ); //send data through serial port
		printf("%d bytes of Data transmitted successfully\n",rwlen);
        /*Description of function WriteFile */
        //hCom : is the handle to SerialPOrt instance
        //send : buffer containing data to be sent
        //n : number of bytes to send
        //rwlen: number of bytes actually sent
        //0 or NULL: i don't know what it is for but its necessary, may be it used in
        //file access because these same functions are also used for fileaccess in windows

        //WriteFile function returns a NULL value in case of error can be used
        //to show pr manage errors.

        //Sleep(3000 ); //just to check the how well the receive buffer works
		//but a small delay is required so that we allow time for the receive buffer to be populated
	/*	Sleep(1000);
		ReadFile( hCom, receive, sizeof(receive), &rwlen, 0 ); // read data from the serial port buffer of the OS
        printf("%d of out of %d bytes read from port and data is %s\n",rwlen,sizeof(receive),receive);
		*/
		int i;
		for(i=0;i<10;i++)
		{
    //from this test i found out that data is only removed from the Serial FIFO buffer
    //of either the OS or serial Device driver(i am not sure, who is in control) when that byte is read
    //until that new data keeps adding up to the buffer until the buffer overflows
    //and i think the buffer overflow point was somewhere near 200 bytes(I tried to read 512bytes atonce)
    //in my setup->pl2303 usb to serial in windows 7

    //Timeouts are absolutely necessary otherwise the program will hang if not able
    //to read specified number of bytes
        strset(receive,0);//clears the string buffer "receive"
		Sleep(1000);
		ReadFile( hCom, receive, sizeof(receive), &rwlen, 0 ); // read data from the serial port buffer of the OS
        printf("%d of out of %d bytes read from port and data is %s\n",rwlen,sizeof(receive),receive);
		}
        /*Description of function ReadFile */
        //hCom : is the handle to SerialPOrt instance
        //receive : buffer into which data it to be read
        //n : number of bytes to read from receive buffer
        //rwlen: number of bytes actually read. It Plays an important role in here as it tells
        //us the actual number of bytes read from the buffer not that is specified to read.

        //small error in my experiment. I found now that when i give number of characters to be read
        //more than that of to be received then it hangs waiting for more characters to arrive into
        //buffer to be read. So if you now the number of characters to be received
        //specify exactly or less but not more, other wise the program will hang
        //oneway to escape from this problem is by using timeout parameter.

        //So the serialport file will have a FIFO buffer which can then be used to
        //read data from. Once you read some characters those characters are removed
        //from the buffer and whatever are remaining to be read will be present in
        //the buffer. Once all characters are read then buffer will be empty.
/*
        Solution for this allready found using timeouts

        //if there would be some mechanism to find how many bytes are present in the buffer
        //then it would be great or one way is to keep reading one character each time(loop) until you get a timeout.
*/
        //but if you are continuosly reading data like in a monitor or terminal app
        //then no problem you can safely specify number of bytes to be read
        //before display and loop it continuously so for example you have specified
        //8bytes to read then ReadFile will exit only after it has read 8bytes or on timeout
        //if timeout is specified.

        //Finally i think using the timeouts worked out as now it doesn't hang even on specifying
        //more values to read.


        //0 or NULL: i don't know what it is for but its necessary, may be it used in
        //file access because these same functions are also used for fileaccess in windows

        //ReadFile function returns a NULL value in case of error can be used
        //to show or manage errors.

//moral of the whole story is that we can't receive data and send data from console
//at the same time obviously because in a console application you can't do both at a time,
//but even if you go for window based application then it would require multithreaded programming,
//or overlapped access or service creation etc. which are much advanced concepts for
//the average experimenters

//so for example i am building a programmer i will give an instruction and i know
//exactly how many bytes i want to read or i can wait for a timeout
//or i can wait for a null character,

/*Text below is only applicable if the FIFO buffer has overflown
//but keep in mind that the serial
//buffer will be overwritten and data will be lost if you don't read
//the data before any new data arrives. */

//Scenario-1
//if you are into making a terminal like stuff then you should learn either the
//above mentioned advanced concepts or use simple timesharing, by which
//we calculate the minimum receive time between characters according to baudrate
//for example for 9600 baudrate deducting the 2% overhead,  minimum receive time
//between consecutive characters will be something close to but less than 1ms
//so we have 1ms of time to read the device.(or better use timeouts)

//So judging from the size of fifo buffer we and time it will take before
//the buffer is filled and our baudrate we can read it in specific intervals
//and do our processing in rest of the time.

//Scenario-2
//if you are the designer of both the embedded device and the PC side software
//then you can code some kind of token based mechanism on receipt of which the
//embedded device will understand that it is now safe to transmit, as the PC side
//app is waiting for response.

	CloseHandle( hCom );//close the handle

	return 0;
}
