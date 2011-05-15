
TCITIZEN         * dummyCitizenGen                         ( void )
 {
   static int count = 50; /* !!! caution here. This global variable is 0 after ThreadedOffice() terminates. !!! */
   TCITIZEN   * n;
   if ( count > 0 )
    {
      count --;
      n = (TCITIZEN *) malloc ( sizeof ( *n ) );
      n -> m_Agenda = AGENDA_ID;
      return ( n );
    }
   return NULL;
 }
void               dummyCitizenDone                        ( TCITIZEN        * citizen,
                                                             int               status )
 {
   free ( citizen );
 }
  
int                dummyProcessID                          ( TREQUESTID      * data )
 {
   sleep ( 1 ); /* processing takes some time */
   return ( 1 ); /* always ok in this dummy implementation */
 }
  
int                dummyProcessCar                         ( TREQUESTCAR     * data )
 {
   sleep ( 2 ); /* processing takes some time */
   return ( 1 ); /* always ok in this dummy implementation */
 }
  
int                dummyProcessTax                         ( TREQUESTTAX     * data )
 {
   sleep ( 5 ); /* processing takes some time */
   return ( 1 ); /* always ok in this dummy implementation */
 }
  
int                dummyRegister                           ( int               agenda )
 {
   return ( 1 ); /* always ok in this dummy implementation */
 }
  
  
void               dummyDeregister                         ( void )
 {
   /* dummy implementation */
 }
  
void               demo                                    ( void )
 {
   TOFFICE  o;
   int      agenda[1] = { AGENDA_CAR | AGENDA_TAX | AGENDA_ID }; /* one universal clerk */

   o . m_CitizenGen  = dummyCitizenGen;
   o . m_CitizenDone = dummyCitizenDone;
   o . m_ProcessID   = dummyProcessID;  
   o . m_ProcessCar  = dummyProcessCar; 
   o . m_ProcessTax  = dummyProcessTax; 
   o . m_Register    = dummyRegister;   
   o . m_Deregister  = dummyDeregister; 
   o . m_ClerkNr     = 1;
   o . m_ClerkAgenda = agenda;
   ThreadedOffice ( &o );

   /* call example: o . m_Register ( AGENDA_LIFTBOY ); */
 }
 
