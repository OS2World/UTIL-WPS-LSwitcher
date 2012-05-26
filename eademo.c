
/* EADemo.c - program to read or write a single-valued ASCII EA */

#define INCL_DOS
#define INCL_DOSERRORS
#include <os2.h>

#include <stdio.h>
#include <stdlib.h>

#include        "EAString.h"

/*****************************************************************************/
/* printrc: print an explanatory message for an OS/2 (or user-defined)       */
/*          error code                                                       */
/*****************************************************************************/

static void printrc( APIRET rc )
   {
   CHAR pchBuf[512] = {'\0'};
   ULONG ulMsgLen = 0;
   APIRET ret = 0;                /* return code from DosGetMessage()        */


   ret = DosGetMessage( NULL, 0, pchBuf, 512, rc, "OSO001.MSG", &ulMsgLen);
   if (ret == 0)
      printf( "%.*s", ulMsgLen, pchBuf );
   else if ( rc == EA_ERROR_NOT_FOUND )
      printf( "EA item was not found" );
   else if ( rc == EA_ERROR_WRONG_TYPE )
      printf( "EA data is not simple ASCII" );
   else
      printf( "OS/2 error code: %u", rc );
   }


/*****************************************************************************/
/* M A I N   P R O G R A M                                                   */
/*****************************************************************************/

int main( int argc, char **argv )
   {
   APIRET rc = 0;
   char szBuf[ 256 ] = {'\0'};    /* arbitrary max length for buffer         */


   if ( ( argc != 3 ) && ( argc != 4 ) )
      {
      printf( "Syntax: EADemo <file> <EAname> [text]\n" );
      exit ( 1 );
      }

   if ( argc == 3 )
      rc = EAQueryString( argv[1], argv[2], sizeof( szBuf ), szBuf );
   else
      rc = EASetString( argv[1], argv[2], argv[3] );

   if ( rc != 0 )
      {
      printf( "Unable to access EA item %s\n", argv[2] );
      printrc( rc );
      printf( "\n" );
      }
   else
      {
      if ( argc == 3 )
         printf( "Value of EA item %s is: \"%s\"\n", argv[2], szBuf );
      else if ( argv[3][0] == '\0' )
         printf( "EA item %s deleted\n", argv[2] );
      else
         printf( "Value of EA item %s set to: \"%s\"\n", argv[2], argv[3] );
      }

   return (int) rc;
   }