<?php
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer
 Date:		Jun 2016
________________________________________________________________________

The settings here are defaults only. Best practice is to copy this file
to dlsitessettings.local.php and put the changes there.

-*/

//Directory where dlsites data should reside
$DLSITES_DATA_ROOT_DIR = '';

//Subdirectory where processing scripts saves processed data
$DLSITES_UNPROCESSED_DIR = 'unprocessed';

//Subdirectory where processing script stores unprocessed input files
$DLSITES_PROCESSED_DIR = 'processed';

//Subdirectory where processing scripts saves processed data
$DLSITES_ARCHIVE_DIR = 'archive';

//Subdirectory where processing scripts saves processed compressed data
$DLSITES_COMPRESSED_ARCHIVE_DIR = 'compressed_archive';

//Site that should be reported as download site
$DLSITES_DOWNLOAD_SITE = 'download.opendtect.org';

//Statistics database host
$DLSITES_DB_HOST = '';

//Statistics database
$DLSITES_DB = '';

//Statistics database user
$DLSITES_DB_USER = '';

//Statistics database password
$DLSITES_DB_PW = '';

//Table prefix
$DLSITES_TABLE_PREFIX = 'dlsites_';

//Period Table prefix
$DLSITES_PERIOD_TABLE_PREFIX = 'period_';

//IP-geography database key
$DLSITES_IP_API_KEY = '';

//IP-geography database key
$DLSITES_USE_LOCAL_IP_DB = false;

//GS archive path
$DLSITES_GS_ARCHIVE_PATH = '';

//GS bucket path
$DLSITES_GS_BUCKET = '';

//GS credentials file
$DLSITES_GS_CREDENTIALS = '';

//Override defaults if local file exists
$localsettingsfile = realpath(dirname(__FILE__))."/dlsitessettings.local.php";
if(file_exists( $localsettingsfile ))
{
    include_once( $localsettingsfile );
}
