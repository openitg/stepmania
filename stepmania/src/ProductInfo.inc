; Included by the NSIS installer script
; Don't forget to also change ProductInfo.h!

!define PRODUCT_FAMILY "StepMania"
; ID is used for the install directory and registry locations
!define PRODUCT_ID "StepMania4"
!define PRODUCT_VER "4.0.b3"
!define PRODUCT_DISPLAY "${PRODUCT_FAMILY} ${PRODUCT_VER}"
!define PRODUCT_BITMAP "sm4"

!define PRODUCT_URL "http://www.stepmania.com"
!define UPDATES_URL "http://www.stepmania.com/wiki/Downloads"

;!define INSTALL_EXTERNAL_PCKS
;!define INSTALL_INTERNAL_PCKS
!define INSTALL_PROGRAM_LIBRARIES
!define INSTALL_EXECUTABLES
!define INSTALL_NON_PCK_FILES
!define ASSOCIATE_SMZIP
!define ALLOW_OPENCANDY
;!define SHOW_AUTORUN
!define MAKE_OPEN_PROGRAM_FOLDER_SHORTCUT
;!define DIRECTX_81_REDIST_PRESENT
;!define MAKE_UPDATES_SHORTCUT
!define CRC_CHECK
!define COMPRESS
