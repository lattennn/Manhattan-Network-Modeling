simple l1t1c
{
    parameters:
        @display("i=block/routing");
    gates:
        input in[];
        output out[];
}

network lab1_task1_c
{
    types:
        channel Channel extends ned.DelayChannel { // Channel delay.
            delay = 10ms;
        }
        
    submodules:
        manh[25]: l1t1c;
    connections:
        for i=0..24 
        {
        	//(i/5) -> Row
        	//(i % 5) -> Column
        	
        	// Horizontal gates are filled first.
        	//If row is odd and column is not four
        	manh[i].out++ --> Channel --> manh[i+1].in++ if ((i/5)%2 != 0) && (i%5!=4);
        	//If row is odd and column is four
        	manh[i].out++ --> Channel --> manh[i-4].in++ if ((i/5)%2 != 0) && (i%5==4);
        	//If row is even and column is zero
	       	manh[i].out++ --> Channel --> manh[i-1].in++ if ((i/5)%2 == 0) && (i%5!=0);
	       	//If row is even and column is zero
        	manh[i].out++ --> Channel --> manh[i+4].in++ if ((i/5)%2 == 0) && (i%5==0);
        	
	        // Vertical gates are filled second.
	        //Module 19 is in column 4, so we disable whenever the column is 4.
	        
	        //OMNeT++ does not return the result of "/" as an int, so we need to substract
        	//the mod of the division, divided by the same dividend in order to check if 
        	//any particular row possesses a certain value.   
        
        	//If column is even and row is not four
           	manh[i].out++ --> Channel --> manh[i+5].in++ if (((i%5)%2 == 0) && ((i/5)-i%5/5)!=4 && (i%5)!=4) ;
        	//If column is even and row is four
        	manh[i].out++ --> Channel --> manh[i-20].in++ if ((i%5)%2 == 0) && (((i/5)-(i%5)/5)==4 && (i%5)!=4); 
        	//If column is odd and row is not zero
        	manh[i].out++ --> Channel --> manh[i-5].in++ if ((i%5)%2 != 0) && (((i/5)-i%5/5)!=0);
        	//If column is odd and row is zero
	       	manh[i].out++ --> Channel --> manh[i+20].in++ if ((i%5)%2 != 0) && (((i/5)-i%5/5)==0); 

        }        	
    	  
}
