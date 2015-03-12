//Geoffrey Grimmer
//Siddharth Perumal
//10/10/2014
#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <vector>
#include <fstream>
#include <string>
#include <list>
#include <algorithm>
using namespace std;


//Adjacency Marix declaration



struct initStruct{
  int level;
  int triggerSearch;
  int ID;
};

struct reportStruct{
  int ID; 
  int edgeWeight;
};

struct testStruct{
  int ID;
  int level;
};

struct acceptStruct{
  int level;
  int ID;
};

struct rejectStruct{
  int level;
};

struct rootStruct{
  int intendedRecipient;
  int level;
  int ID;
};

struct connectStruct{
  int ID;
  int level;
};

struct component{
  int level;
  int leaderID;
};
  

struct messageStruct{
  string messageType;
  int sender;
  union{
    struct initStruct init;
    struct reportStruct report;
    struct testStruct test;
    struct acceptStruct accept;
    struct rejectStruct reject;
    struct rootStruct root;
    struct connectStruct connect;
  };

};



vector<vector<int> > adjMatrix;
vector<vector<string> > edgeMatrix;
vector< vector< list<struct messageStruct> > > messageMatrix;
vector<vector<signed int> > convergeMatrix;
vector<vector<list< int> > > delayMatrix; //can use to check if all the messages a process needs have been sent
vector<int> parentRelationship; 
vector<int> leader;
pthread_mutex_t lock;
pthread_mutex_t init;



//Control variables
int threadCount, writing=10000 ,stepDone =0, terminateMet =0, nextStep=0,exitAll=0, threadHold=1, stepStart=0, threadStart =1, doneWriting=0;
volatile int terminateProg=0;
int *threadIdetifier;


list<messageStruct > check_messages(int localTID, vector<int> myNeighbors){
  list<struct messageStruct > messagesReady;
  for(unsigned int ii=0; ii<myNeighbors.size(); ii++){
    unsigned int jj = myNeighbors[ii] - 1;
    if((!delayMatrix[jj][localTID-1].empty()) && (*(delayMatrix[jj][localTID-1].begin())==0)){
      struct messageStruct value;
      value = *messageMatrix[jj][localTID-1].begin();
      messageMatrix[jj][localTID-1].pop_front();
      delayMatrix[jj][localTID-1].pop_front();
      messagesReady.push_back(value);
    }
  }

  return messagesReady;
}


int send_report_message(int sender, int edgeWeight, int ID, int recipient){
  int delay = rand() % 20 + 1; 
  struct messageStruct tempReportMessage;
  tempReportMessage.sender=sender;
  tempReportMessage.messageType="REPORT";
  tempReportMessage.report.ID=ID;
  tempReportMessage.report.edgeWeight=edgeWeight;

  messageMatrix[sender-1][recipient-1].push_back(tempReportMessage);
  delayMatrix[sender-1][recipient-1].push_back(delay);

  return 1;

}  
int send_initiate_message(int localTID, int branch, int leaderID, int level, int triggerSearch){
  int delay = rand() % 20 + 1; 
  struct messageStruct tempInit;
  tempInit.sender=localTID;
  tempInit.messageType="INITIATE";
  tempInit.init.level=level;
  tempInit.init.ID=leaderID;
  tempInit.init.triggerSearch=triggerSearch;
  
  messageMatrix[localTID-1][branch-1].push_back(tempInit);
  delayMatrix[localTID-1][branch-1].push_back(delay);

  return 1;
}


int send_connect_message(int sender, int recipient, int level, int ID){
  int delay = rand() % 20 + 1; 
  struct messageStruct tempConnectMessage;
  tempConnectMessage.sender=sender;
  tempConnectMessage.messageType="CONNECT";
  tempConnectMessage.connect.ID=ID;
  tempConnectMessage.connect.level=level;
  
  messageMatrix[sender-1][recipient-1].push_back(tempConnectMessage);
  delayMatrix[sender-1][recipient-1].push_back(delay);

  return 1;

}  



int send_accept_message(int sender, int recipient, int ID, int level){
  int delay = rand() % 20 + 1; 
  struct messageStruct tempAcceptMessage;
  tempAcceptMessage.sender=sender;
  tempAcceptMessage.messageType="ACCEPT";
  tempAcceptMessage.accept.ID=ID;
  tempAcceptMessage.accept.level=level;
  messageMatrix[sender-1][recipient-1].push_back(tempAcceptMessage);
  delayMatrix[sender-1][recipient-1].push_back(delay);

  return 1;

}

int send_reject_message(int sender, int recipient){
  int delay = rand() % 20 + 1; 
  struct messageStruct tempRejectMessage;
  tempRejectMessage.sender=sender;
  tempRejectMessage.messageType="REJECT";
  
  messageMatrix[sender-1][recipient-1].push_back(tempRejectMessage);
  delayMatrix[sender-1][recipient-1].push_back(delay);

  return 1;
} 

int send_test_message(int sender, int recipient, int ID, int level){
  int delay = rand() % 20 + 1; 
  struct messageStruct tempTestMessage;
  tempTestMessage.test.ID=ID;
  tempTestMessage.test.level=level;
  tempTestMessage.sender=sender;
  tempTestMessage.messageType="TEST";
  messageMatrix[sender-1][recipient-1].push_back(tempTestMessage);
  delayMatrix[sender-1][recipient-1].push_back(delay);
  
  return 1;
}

int send_change_root(int localTID, int recipient, struct reportStruct report_message){
  int delay = rand() % 20 + 1; 
  struct messageStruct tempRootMessage;
  int sender;
  
  tempRootMessage.messageType="CHANGEROOT";
  tempRootMessage.root.intendedRecipient = report_message.ID;


  messageMatrix[localTID-1][recipient-1].push_back(tempRootMessage);
  delayMatrix[localTID-1][recipient-1].push_back(delay);
  
  return 1;  


}


struct reportStruct find_min(int localTID, vector<struct messageStruct> receivedReports){
  struct reportStruct tempReport;
  int min=-1;
  for(vector<struct messageStruct>::iterator it = receivedReports.begin(); it != receivedReports.end(); ++it){
    if(((it->report.edgeWeight < min) || (min == -1)) && (it->report.edgeWeight != -1)){
      min = it->report.edgeWeight;
      tempReport.edgeWeight=min;
      tempReport.ID=it->report.ID;
    }
  }
  
  if(min == -1){
    tempReport.edgeWeight=-1;
    tempReport.ID=-1;
  }


  return tempReport;
}

int tempID;

bool sortFunction(int ii, int jj){
  return adjMatrix[tempID-1][ii-1] < adjMatrix[tempID-1][jj-1];
}



int check_test(int localLeaderID, int localLevel, int testID, int testLevel){
  if(localLeaderID == testID){
    return -1;
  } else if(localLevel >= testLevel){
    return 1;
  } else {
    return 0;
  }

  
}

void *asynch(void *arg)
{
  //read the adjacmency matrix to find neighbours
  //send id to it's neighbours
  list<struct messageStruct > messagesReceived;
  vector<int> myInput = *(vector<int> *) arg;
  vector<int> myLocal = myInput;
  vector<int> myNeighbors (myLocal.begin()+2, myLocal.end()); 
  vector<int> children;
  vector<struct messageStruct> receivedReports;
  children.clear();
  int localTID = myLocal[0];
  //cout << "my TID "<<myLocal[0]<< " my local[1]: "<< myLocal[1] << "\n";

  
  struct component myComponent;
  myComponent.level=0;
  myComponent.leaderID=localTID;
  string myState="INITIAL";
  parentRelationship[localTID-1] = localTID;
  vector<int>::iterator branchIterator = myNeighbors.begin();
  tempID=localTID;
  vector<struct messageStruct> delayedTests;
  // need to sort the neighbor IDS by minimum weight edges
  sort(myNeighbors.begin(), myNeighbors.end(), sortFunction);
  struct messageStruct acceptedEdge;
  pthread_mutex_unlock(&init); 

  while(exitAll != 1)
    {
      

      
      pthread_mutex_lock(&lock); 
      stepStart++;
      list<struct messageStruct > tempMessages=check_messages(localTID, myNeighbors);
      messagesReceived.insert(messagesReceived.end(), tempMessages.begin(), tempMessages.end());
      pthread_mutex_unlock(&lock); 

      while(threadStart == 1);


      if(myState=="INITIAL"){
	//I have no children and am my own leader, no need to send an initiate
	//send a connect message and wait for it to come back?

	int branch=*branchIterator;
	pthread_mutex_lock(&lock); 
	send_connect_message(localTID, branch, myComponent.level, myComponent.leaderID);
	pthread_mutex_unlock(&lock); 
	myState ="CONNECT_OUT";
      }

      if(myState=="START"){
	//waiting for initiate
	for(list<struct messageStruct>::iterator it = messagesReceived.begin(); it != messagesReceived.end(); ++it){
	  if(it->messageType == "INITIATE"){
	    myComponent.level = it->init.level;
	    myComponent.leaderID = it->init.ID;
	    parentRelationship[localTID-1] = it->sender;
	    for(vector<int>::iterator n_it=myNeighbors.begin(); n_it != myNeighbors.end(); ++n_it){
	      if((parentRelationship[localTID-1] != *n_it) && (edgeMatrix[localTID-1][*n_it-1] == "BRANCH")){
		//if it is a branch edge and the edge does not belong to my parent, send the initate message. triggers search
		send_initiate_message(localTID, *n_it , myComponent.leaderID, myComponent.level, it->init.triggerSearch);		  
	      }
	    }

	    if(it->init.triggerSearch){
	      myState = "TEST_FOR_MWOE";
	    } else {
	      myState = "START";
	    }
	    
	    messagesReceived.erase(it);
	    it--;
	  } else if(it->messageType == "TEST"){
	    int accept = check_test(myComponent.leaderID, myComponent.level, it->test.ID, it->test.level);
	    //accept == -1 on reply reject
	    //accept == 0 defer
	    //accept == 1 on reply accept
	    if(accept == -1){
	      if(edgeMatrix[localTID-1][it->sender-1] == "BASIC"){
		edgeMatrix[localTID-1][it->sender-1] = "REJECTED";
	      }
	      send_reject_message(localTID, it->sender);
	      messagesReceived.erase(it);
	      it--;
	    } else if(accept == 0){
	      
	      
	    } else if(accept ==1){
	      send_accept_message(localTID, it->sender, myComponent.leaderID, myComponent.level);
	      messagesReceived.erase(it);
	      it--;
	    }
	    

	  } else if(it->messageType == "CONNECT"){
	    if(myComponent.level > it->connect.level){
	      edgeMatrix[localTID-1][it->sender - 1] = "BRANCH";
	      send_initiate_message(localTID, it->sender , myComponent.leaderID, myComponent.level, 0);
	      messagesReceived.erase(it);
	      it--;
	    }
	  }

	}
      }

      if(myState == "TEST_FOR_MWOE"){
	for(list<struct messageStruct>::iterator it = messagesReceived.begin(); it != messagesReceived.end(); ++it){
	  if(it->messageType == "TEST"){
	    int accept = check_test(myComponent.leaderID, myComponent.level, it->test.ID, it->test.level);
	    //accept == -1 on reply reject
	    //accept == 0 defer
	    //accept == 1 on reply accept
	    if(accept == -1){
	      if(edgeMatrix[localTID-1][it->sender-1] == "BASIC"){
		edgeMatrix[localTID-1][it->sender-1] = "REJECTED";
	      }
	      send_reject_message(localTID, it->sender);
	      messagesReceived.erase(it);
	      it--;
	    } else if(accept == 0){
	      
	      
	    } else if(accept ==1){
	      send_accept_message(localTID, it->sender, myComponent.leaderID, myComponent.level);
	      messagesReceived.erase(it);
	      it--;
	    }
	    
	    
	  } else if(it->messageType == "CONNECT"){
	    if(myComponent.level > it->connect.level){
	      edgeMatrix[localTID-1][it->sender - 1] = "BRANCH";
	      send_initiate_message(localTID, it->sender , myComponent.leaderID, myComponent.level, 1);
	      messagesReceived.erase(it);
	      it--;
	    }
	  }	
	  
	  
	}
	int noMoreBasicEdges=1;
	for(branchIterator=myNeighbors.begin();branchIterator != myNeighbors.end(); ++branchIterator){
	  if(edgeMatrix[localTID-1][*branchIterator-1] == "BASIC"){
	    noMoreBasicEdges=0;
	    break;
	  }
	}
	if(noMoreBasicEdges==1){
	  //I have no more basic edges and therefore want to jump to a reporting state
	  //and my report will be -1
	  acceptedEdge.sender=localTID;
	  acceptedEdge.messageType = "ACCEPT";
	  acceptedEdge.accept.level=-1;
	  acceptedEdge.accept.ID=-1;
	  myState="WAIT_FOR_REPORTS";
	} else {
	  myState="WAIT_FOR_TEST_REPLY";
	  send_test_message(localTID, *branchIterator, myComponent.leaderID, myComponent.level);
	}
      }


      if(myState=="WAIT_FOR_TEST_REPLY"){
	//waiting for reply from test.
	for(list<struct messageStruct>::iterator it = messagesReceived.begin(); it != messagesReceived.end(); ++it){
	  if(it->messageType == "ACCEPT"){
            //If leaf then send the report to parent and go to CONNECT_FOR_CHANGE_ROOT state.
            int skipToWaitnOnChangeRoot = 1;  
	    for(vector<int>::iterator n_it=myNeighbors.begin(); n_it != myNeighbors.end(); ++n_it){
	      if((parentRelationship[localTID-1] != *n_it) && (edgeMatrix[localTID-1][*n_it-1] == "BRANCH")){
		skipToWaitnOnChangeRoot = 0;
		break;
	      }
	    }	    
	    
            if(skipToWaitnOnChangeRoot == 0){
	      //edge is accepted, need to wait for non parent branch edges to send a report back.
	      acceptedEdge = *it;
	      messagesReceived.erase(it);
	      it--;
	      myState = "WAIT_FOR_REPORTS";
            } else{
              //Send message to parent.
              send_report_message(localTID, it->report.edgeWeight, localTID, parentRelationship[localTID-1]);  
              myState = "WAIT_FOR_CHANGE_ROOT";   
	      messagesReceived.erase(it);
	      it--;
            }
            //Store the message information before erase.
            //messagesReceived.erase(it);
	  }else if(it->messageType == "REJECT"){
            //Mark edge as rejected and go back to find a new MWOE.
	    if(edgeMatrix[localTID-1][it->sender - 1] == "BASIC"){
	      edgeMatrix[localTID-1][it->sender - 1] = "REJECTED";  
	    }
	    messagesReceived.erase(it);
	    it--;
            myState = "TEST_FOR_MWOE";
          } else if(it->messageType == "TEST"){
	    int accept = check_test(myComponent.leaderID, myComponent.level, it->test.ID, it->test.level);
	    //accept == -1 on reply reject
	    //accept == 0 defer
	    //accept == 1 on reply accept
	    if(accept == -1){
	      if(edgeMatrix[localTID-1][it->sender-1] == "BASIC"){
		edgeMatrix[localTID-1][it->sender-1] = "REJECTED";
	      }
	      send_reject_message(localTID, it->sender);
	      messagesReceived.erase(it);
	      it--;
	    } else if(accept == 0){
	      //Do nothing as we are defering reply.
	      
	    } else if(accept ==1){
	      send_accept_message(localTID, it->sender, myComponent.leaderID, myComponent.level);
	      messagesReceived.erase(it);
	      it--;
	    }
	    

	  } else if(it->messageType == "CONNECT"){
	    if(myComponent.level > it->connect.level){
	      edgeMatrix[localTID-1][it->sender - 1] = "BRANCH";
	      send_initiate_message(localTID, it->sender, myComponent.leaderID, myComponent.level, 1);
	      messagesReceived.erase(it);
	      it--;
	    }
	  }

	}
      }

      if(myState=="WAIT_FOR_REPORTS"){
	unsigned int reportsExpected=0;
	for(vector<int>::iterator n_it=myNeighbors.begin(); n_it != myNeighbors.end(); ++n_it){
	  if((parentRelationship[localTID-1] != *n_it) && (edgeMatrix[localTID-1][*n_it-1] == "BRANCH")){
	    reportsExpected++;
	  }
	}
	
	for(list<struct messageStruct>::iterator it = messagesReceived.begin(); it != messagesReceived.end(); ++it){
	  reportsExpected=0;
	  for(vector<int>::iterator n_it=myNeighbors.begin(); n_it != myNeighbors.end(); ++n_it){
	    if((parentRelationship[localTID-1] != *n_it) && (edgeMatrix[localTID-1][*n_it-1] == "BRANCH")){
	      reportsExpected++;
	    }
	  }
	  
	  if(it->messageType == "REPORT"){
	    receivedReports.push_back(*it);
	    messagesReceived.erase(it);
	    it--;
	  } else if(it->messageType == "TEST"){
	    int accept = check_test(myComponent.leaderID, myComponent.level, it->test.ID, it->test.level);
	    //accept == -1 on reply reject
	    //accept == 0 defer
	    //accept == 1 on reply accept
	    if(accept == -1){
	      if(edgeMatrix[localTID-1][it->sender-1] == "BASIC"){
		edgeMatrix[localTID-1][it->sender-1] = "REJECTED";
	      }
	      send_reject_message(localTID, it->sender);
	      messagesReceived.erase(it);
	      it--;
	    } else if(accept == 0){
	      //Do nothing as we are defering reply.
	      
	    } else if(accept ==1){
	      send_accept_message(localTID, it->sender, myComponent.leaderID, myComponent.level);
	      messagesReceived.erase(it);
	      it--;
	    }
	    

	  } else if(it->messageType == "CONNECT"){
	    if(myComponent.level > it->connect.level){
	      edgeMatrix[localTID-1][it->sender - 1] = "BRANCH";
	      send_initiate_message(localTID, it->sender , myComponent.leaderID, myComponent.level, 1);
	      messagesReceived.erase(it);
	      it--;
	      myState="WAIT_FOR_REPORTS";
	    }
	  }



	}
	   
	if(receivedReports.size() == reportsExpected){
	 
	  struct messageStruct tempReport; 
	  tempReport.report.ID = localTID;
	  if(acceptedEdge.accept.ID == -1){
	  tempReport.report.edgeWeight = -1;	    
	  } else {
	    tempReport.report.edgeWeight = adjMatrix[localTID-1][acceptedEdge.sender-1];
	  }

	  receivedReports.push_back(tempReport);
	  

	  if(myComponent.leaderID != localTID){
	    myState="WAIT_FOR_CHANGE_ROOT";
	    struct reportStruct tmpReport=find_min(localTID, receivedReports);
	    send_report_message(localTID, tmpReport.edgeWeight, tmpReport.ID, parentRelationship[localTID-1]);

	  } else {
	    myState="START";
	    struct reportStruct tmpReport=find_min(localTID, receivedReports);
	    if(tmpReport.edgeWeight == -1){
	      terminateProg=1;
	    }



	  }

	  if(myComponent.leaderID == localTID){
	    struct reportStruct tmpReport=find_min(localTID, receivedReports);
	    for(vector<int>::iterator n_it=myNeighbors.begin(); n_it != myNeighbors.end(); ++n_it){
	      if((parentRelationship[localTID-1] != *n_it) && (edgeMatrix[localTID-1][*n_it-1] == "BRANCH")){
		send_change_root(localTID, *n_it, tmpReport);
	      }
	      
	    }

	    if(tmpReport.ID == localTID){
	      myState="CONNECT_OUT";
	      send_connect_message(localTID, *branchIterator, myComponent.level, myComponent.leaderID);
	    }
	  }

	  receivedReports.clear();
	}



      }
      
      

      if(myState=="WAIT_FOR_CHANGE_ROOT"){
	for(list<struct messageStruct>::iterator it = messagesReceived.begin(); it != messagesReceived.end(); ++it){
	  if(it->messageType == "CHANGEROOT"){
	    for(vector<int>::iterator n_it=myNeighbors.begin(); n_it != myNeighbors.end(); ++n_it){
	      if((parentRelationship[localTID-1] != *n_it) && (edgeMatrix[localTID-1][*n_it-1] == "BRANCH")){
		struct reportStruct tmpReport;
		tmpReport.ID=it->report.ID;
		send_change_root(localTID,*n_it, tmpReport);
	      }
	    }
	    
	    if(it->report.ID == localTID){
	      myState="CONNECT_OUT";
	      send_connect_message(localTID, *branchIterator, myComponent.level, myComponent.leaderID);
	    } else {
	      myState="START";
	    }
	    messagesReceived.erase(it);
	    it--;

	  } else if(it->messageType == "TEST"){
	    int accept = check_test(myComponent.leaderID, myComponent.level, it->test.ID, it->test.level);
	    //accept == -1 on reply reject
	    //accept == 0 defer
	    //accept == 1 on reply accept
	    if(accept == -1){
	      if(edgeMatrix[localTID-1][it->sender-1] == "BASIC"){
		edgeMatrix[localTID-1][it->sender-1] = "REJECTED";
	    }
	      send_reject_message(localTID, it->sender);
	      messagesReceived.erase(it);
	      it--;
	    } else if(accept == 0){
	      //Do nothing as we are defering reply.
	      
	    } else if(accept ==1){
	      send_accept_message(localTID, it->sender, myComponent.leaderID, myComponent.level);
	      messagesReceived.erase(it);
	      it--;
	    }
	    

	  } else if(it->messageType == "CONNECT"){
	    if(myComponent.level > it->connect.level){
	      edgeMatrix[localTID-1][it->sender - 1] = "BRANCH";
	      send_initiate_message(localTID, it->sender , myComponent.leaderID, myComponent.level, 1);
	      messagesReceived.erase(it);
	      it--;
	      myState="WAIT_FOR_REPORTS";
	    }
	  }




	}
      }


      if(myState == "CONNECT_OUT"){
	for(list<struct messageStruct>::iterator it = messagesReceived.begin(); it != messagesReceived.end(); ++it){
	  if(it->messageType == "INITIATE"){
	    //absorb
	    myComponent.level = it->init.level;
	    myComponent.leaderID = it->init.ID;
	    parentRelationship[localTID-1] = it->sender;
	    edgeMatrix[localTID-1][*branchIterator-1] = "BRANCH";
	    if(it->init.triggerSearch){
	      //FIND MWOE
	      myState="TEST_FOR_MWOE";
	      for(vector<int>::iterator n_it=myNeighbors.begin(); n_it != myNeighbors.end(); ++n_it){
		if((parentRelationship[localTID-1] != *n_it) && (edgeMatrix[localTID-1][*n_it-1] == "BRANCH")){
		  //if it is a branch edge and the edge does not belong to my parent, send the initate message. triggers search
		  send_initiate_message(localTID, *n_it , myComponent.leaderID, myComponent.level, 1);		  
		}
	      }

	    } else{
	      //wait for initiate
	      myState="START";
	      for(vector<int>::iterator n_it=myNeighbors.begin(); n_it != myNeighbors.end(); ++n_it){
		if((parentRelationship[localTID-1] != *n_it) && (edgeMatrix[localTID-1][*n_it-1] == "BRANCH")){
		  //if it is a branch edge and the edge does not belong to my parent, send the initate message. DO NOT TRIGGER SEARCH
		  send_initiate_message(localTID, *n_it , myComponent.leaderID, myComponent.level, 0);		  
		}
	      }

	    }
	      
	    messagesReceived.erase(it);
	    it--;
	    branchIterator++;
	  } else if(it->messageType == "CONNECT"){
	    if(*branchIterator == it->sender){
	      //merge
	      myComponent.level++;
	      if(localTID < *branchIterator){
		myComponent.leaderID = *branchIterator;
		parentRelationship[localTID-1] = *branchIterator;
		myState="START";		
		edgeMatrix[localTID-1][*branchIterator-1] = "BRANCH";
	      } else {
		myComponent.leaderID = localTID;
		parentRelationship[localTID-1] = localTID;
		myState="TEST_FOR_MWOE";		
		edgeMatrix[localTID-1][*branchIterator-1] = "BRANCH";
		for(vector<int>::iterator n_it=myNeighbors.begin(); n_it != myNeighbors.end(); ++n_it){
		  if(edgeMatrix[localTID-1][*n_it-1] == "BRANCH"){
		    send_initiate_message(localTID, *n_it, myComponent.leaderID, myComponent.level, 1);
		  }
		}	

	      
	      }
	      branchIterator++;
	    } else {
	      edgeMatrix[localTID-1][it->sender-1]="BRANCH";
	      send_initiate_message(localTID, it->sender , myComponent.leaderID, myComponent.level, 0);	      
	    }

	    messagesReceived.erase(it);
	    it--;
	  } else if(it->messageType == "TEST"){
	    int accept = check_test(myComponent.leaderID, myComponent.level, it->test.ID, it->test.level);
	    //accept == -1 on reply reject
	    //accept == 0 defer
	    //accept == 1 on reply accept
	    if(accept == -1){
	      if(edgeMatrix[localTID-1][it->sender-1] == "BASIC"){
		edgeMatrix[localTID-1][it->sender-1] = "REJECTED";
	      }
	      send_reject_message(localTID, it->sender);
	      messagesReceived.erase(it);
	      it--;
	    } else if(accept == 0){
	      
	      
	    } else if(accept ==1){
	      send_accept_message(localTID, it->sender, myComponent.leaderID, myComponent.level);
	      messagesReceived.erase(it);
	      it--;
	    }

	  }
	
	}
      
      }

      







      int ready=0;
      for(vector<int>::iterator temp_it=myNeighbors.begin(); temp_it!=myNeighbors.end(); ++temp_it){
	if(edgeMatrix[localTID-1][(*temp_it)-1] == "BASIC"){
	  ready=0;
	  break;
	}
	ready=1;

      }
      if(ready==1){
	pthread_mutex_lock(&lock); 
	//	terminateMet++;
	pthread_mutex_unlock(&lock); 
      }

      pthread_mutex_lock(&lock); 
      stepDone++;
      pthread_mutex_unlock(&lock); 
      while(threadHold ==1);
      //    cout << "my ID: " << localTID << " my delay is: " << delay<<"\n";

      //      terminateProg=1;
    }
  return (void *) 1;
}



int main(int argc, char**argv) 
{
  unsigned int n;
  //  //cout<<"Enter the number of processes in the distributed system:";
  //cin>>n;
  ////cout<<endl;
  string line;
  ifstream myfile ("input.txt");
  //  ofstream myOut ("output.txt");
  unsigned int linecount=0;


  

  while(1){
    if(myfile.is_open())
      {
	if(linecount == 0){
	  //	n=atoi(line.c_str());
	  myfile >> n;
	  //cout << "\n"<<n<<"\n";
	  adjMatrix.resize(n);
	  messageMatrix.resize(n);
	  edgeMatrix.resize(n);
	  delayMatrix.resize(n);
	  convergeMatrix.resize(n);
	  parentRelationship.resize(n);
	  leader.resize(n);
	  for(unsigned int i=0; i<n; i++){
	    adjMatrix[i].resize(n);
	    convergeMatrix[i].resize(n);
	    messageMatrix[i].resize(n);
	    edgeMatrix[i].resize(n);
	    delayMatrix[i].resize(n);
	  }

	} else {
	  myfile >> adjMatrix[(linecount-1)/n][(linecount-1)%n];
	    
	  //cout << adjMatrix[(linecount-1)/n][(linecount-1)%n];
	 
	  
	}
	if(linecount==n*n){
	  break;	
	}
	linecount++;
	
      }
  }

    
  //  adjMatrix[1][1] = 1;
  pthread_t thread[n+1];
  threadCount = n;
    
  if(pthread_mutex_init(&lock,NULL) != 0)
    {
      //cout<<"Mutex lock initialization failed!!!"<<endl;
      return 1;
    }
  
  if(pthread_mutex_init(&init,NULL) != 0)
    {
      //cout<<"Mutex lock initialization failed!!!"<<endl;
      return 1;
    }  
  //Control thread
  //pthread_create(&thread[0],NULL,controlThread,(void*) n);

  //Threads created for simulating the processes of the distributed system.

  vector<int> adjList;
  for(unsigned int i=1;i<=n;i++)
    {
      pthread_mutex_lock(&init);   
      adjList.clear();
      int adjCount=0;
      for(unsigned int j=0; j<n; j++){
	if(adjMatrix[i-1][j] != 0){
	  edgeMatrix[i-1][j]="BASIC";
	  adjCount++;
	  adjList.push_back(j+1);
	}
      }
      threadIdetifier = (int*) malloc(sizeof(int));
      *threadIdetifier = i;
      adjList.insert(adjList.begin(), adjCount);
      adjList.insert(adjList.begin(), i);
      
      pthread_create(&thread[i],NULL,asynch, (void *) &adjList);
  
    }
  //Wait till all threads are completed.
  pthread_mutex_lock(&init);

  while(terminateProg==0)
    {
      while(stepStart<threadCount);
      pthread_mutex_lock(&lock);
      threadHold=1;
      stepStart=0;
      threadStart=0;
      terminateMet=0;
      pthread_mutex_unlock(&lock);      
      

      while(stepDone<threadCount);
      //decrement the messages
      pthread_mutex_lock(&lock);
     

      for(unsigned int ii=0; ii<n; ii++){
	for(unsigned int jj=0; jj<n; jj++){
	  for(list<int>::iterator delayIT=delayMatrix[ii][jj].begin(); delayIT != delayMatrix[ii][jj].end(); ++delayIT){
	    if(*delayIT > 0){
	      *delayIT=*delayIT-1;
	    }
	  } 
	}
      }
      threadStart=1;
      stepDone=0;
      threadHold=0;
      pthread_mutex_unlock(&lock);

      
        
    }
  exitAll =1;
  threadHold=0;
  threadStart=0;
  pthread_mutex_lock(&lock);
  for(unsigned int i=1; i<=n; i++){
    if(parentRelationship[i-1]==i)
      cout << "proccess ID: " << i << " IS THE ROOT\n";

  }
  for(unsigned int i=1; i<=n; i++){
    if(parentRelationship[i-1]!=i)
      cout << "proccess ID: " << i << " and my parent is: " << parentRelationship[i-1] << "\n";
    
  }
  pthread_mutex_unlock(&lock);

  for(unsigned int i=1;i<=n;i++)
    {

      pthread_join(thread[i],NULL);
    }


  return 0;
}
