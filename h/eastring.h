/* Taken from an excellent article by Roger Orr dating back to 1993 */
/* Header file for EAString.c module */

APIRET EAQueryString( PSZ pszPathName, PSZ pszEAName, USHORT cbBuf, PSZ pszBuf );
APIRET EASetString( PSZ pszPathName, PSZ pszEAName, PSZ pszBuf );

#define EA_ERROR_NOT_FOUND              (ERROR_USER_DEFINED_BASE + 1)
#define EA_ERROR_WRONG_TYPE             (ERROR_USER_DEFINED_BASE + 2)

