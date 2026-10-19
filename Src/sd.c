/*
 * sd.c
 *
 *  Created on: May 31, 2024
 *      Author: Selina
 */

#include "sd.h"

SPI_HandleTypeDef SD_SPI_HANDLE;

FATFS FatFs; 	//Fatfs handle
//FIL File;

FIL file;
DIR dirs;

FRESULT res;

extern UART_HandleTypeDef huart4;

void myprintf(const char *fmt, ...) {
  static char buffer[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  int len = strlen(buffer);
  HAL_UART_Transmit(&huart4, (uint8_t*)buffer, len, -1);
}

FRESULT card_init(sd_card_t *_sd, SPI_HandleTypeDef *sd_hspi){
//	HAL_Delay(1000);
	FRESULT fres;
	SD_SPI_HANDLE = *sd_hspi;

	if(HAL_GPIO_ReadPin(SD_CD_GPIO_Port, SD_CD_Pin) == 0){
		_sd->card_detect = 1;
		fres = f_mount(&FatFs, "/", 1);
		if(fres == FR_OK){
			_sd->card_mouted = 1;
			fres = get_free_space(_sd);
		}
		else{
			_sd->card_mouted = 0;
		}
	}
	else{
		_sd->card_detect = 0;
		_sd->card_mouted = 0;
		fres = FR_INT_ERR;
	}

	return fres;
}

FRESULT card_unmount(sd_card_t *_sd){
	FRESULT fres = f_mount(NULL, "", 0);
	if(fres == FR_OK){
		_sd->card_mouted = 0;
		_sd->free_sectors_KiB = 0;
		_sd->total_sectors_KiB = 0;
	}
	return fres;
}

FRESULT card_remount(sd_card_t *_sd){
	MX_FATFS_Init();
	FRESULT fres = f_mount(&FatFs, "/", 1);
	if(fres != FR_OK){
		MX_FATFS_DeInit();
		card_unmount(_sd);
		_sd->card_mouted = 0;
	}
	else{
		_sd->card_mouted = 1;
		fres = get_free_space(_sd);
	}
	return fres;
}
FRESULT fres;
FRESULT get_free_space(sd_card_t *_sd){
	DWORD free_clusters;
	FATFS* getFreeFs;
	fres = f_getfree("", &free_clusters, &getFreeFs);

	_sd->total_sectors_KiB = ((getFreeFs->n_fatent - 2) * getFreeFs->csize) / 2;
	_sd->free_sectors_KiB = (free_clusters * getFreeFs->csize) / 2;

	return fres;
}


int count_mark;
char mainpath[20];
FRESULT file_open(sd_card_t *_sd, char *path, char *filename, BYTE mode){
	res = f_opendir(&dirs, path);
	if(res == FR_NO_PATH){
		int foundMainPath = 0;
		count_mark = 0;
		memset(mainpath, 0, sizeof(mainpath));

		for(int i = 0; i <= strlen(path); ++i){
			if(path[i] == '/') ++count_mark;
			if(count_mark < 2){
				mainpath[foundMainPath] = path[i];
				++foundMainPath;
			}
			if(count_mark == 2) break;
		}
		res = f_mkdir(mainpath);
		if((res == FR_OK) || (res == FR_EXIST)){
			if(count_mark == 2){
				res = f_mkdir(path);
				if(res == FR_OK){
					goto OPEN_FILE;
				}
				else {
					_sd->file_opened = 0;
					goto FILE_OPEN_RETURN;
				}
			}
			goto OPEN_FILE;
		}
		else {
			_sd->file_opened = 0;
			goto FILE_OPEN_RETURN;
		}
	}
OPEN_FILE:
	char file_link[100];
	sprintf(file_link, "%s/%s", path, filename);
	res = f_open(&file, file_link, mode);
	if(res == FR_OK){
		_sd->file_opened = 1;
	}
	else _sd->file_opened = 0;
FILE_OPEN_RETURN:
	return res;
}

FRESULT file_close(sd_card_t *_sd){
	if(_sd->file_opened){
		res = f_close(&file);
		if(res == FR_OK) _sd->file_opened = 0;
	}
	return res;
}

FRESULT file_write(sd_card_t *_sd, char *data){
	if(_sd->file_opened == 1){
		UINT bw;
		f_lseek(&file, f_size(&file));
		res = f_write(&file, data, strlen(data), &bw);
	}
	return res;
}

int file_size(){
	return f_size(&file);
}


char list_folder[40][20];

FRESULT find_folder_name(sd_card_t *_sd, const char *path, uint16_t ordinal, char *folder_name){
	FRESULT res;

	res = list_dir(path);
	memset(folder_name, 0, strlen(folder_name));
	strcpy(folder_name, list_folder[ordinal - 1]);

	return res;
}

FRESULT delete_folder(char *path){
	FRESULT res;
	DIR dir;
	FILINFO fno;
//	int nfile, ndir;

	res = f_opendir(&dir, path);                       /* Open the directory */
	if (res == FR_OK) {
//		nfile = ndir = 0;
		for (;;) {
			res = f_readdir(&dir, &fno);                   /* Read a directory item */
			if (res != FR_OK || fno.fname[0] == 0) break;  /* Error or end of dir */
			if (fno.fattrib & AM_DIR) {            /* Directory */
//				myprintf("   <DIR>   %s\n", fno.fname);
				f_closedir(&dir);

				char subfolder_to_delete[100];
				memset(subfolder_to_delete, 0, sizeof(subfolder_to_delete));
				sprintf(subfolder_to_delete,"%s/%s", path, fno.fname);

				delete_folder(subfolder_to_delete);

				res = f_opendir(&dir, path);
//				strcpy(list_folder[ndir],fno.fname);
//				ndir++;
			}
			else {                               /* File */
//				myprintf("%10u %s\n", fno.fsize, fno.fname);
				char link_to_delete[100];
				memset(link_to_delete, 0, sizeof(link_to_delete));
				sprintf(link_to_delete,"%s/%s", path, fno.fname);
				res = f_unlink(link_to_delete);
			}
		}
		f_closedir(&dir);
		res = f_unlink(path);
//		myprintf("%d dirs, %d files.\n", ndir, nfile);
	}
//	else {
//		myprintf("Failed to open \"%s\". (%u)\n", path, res);
//	}
	return res;

//	FRESULT res;
//    DIR dir;
//	res = f_opendir(&dir, path);
//	res = f_unlink(path);
//	return res;
}


void swap(char *x, char *y) {
  // Khai báo một biến tạm thời để lưu trữ giá trị của x
  char temp[1000];
  // Sao chép nội dung của x sang temp
  memset(temp, 0, sizeof(temp));
  strcpy(temp, &*x);
  // Sao chép nội dung của y sang x
  memset(x, 0, strlen(x));
  strcpy(&*x, &*y);
  // Sao chép nội dung của temp (giá trị ban đầu của x) sang y
  memset(y, 0, strlen(y));
  strcpy(&*y, temp);
}

FRESULT list_dir (const char *path)
{
    FRESULT res;
    DIR dir;
    FILINFO fno;
    int nfile, ndir;

    res = f_opendir(&dir, path);                       /* Open the directory */
    if (res == FR_OK) {
        nfile = ndir = 0;
        for (;;) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Error or end of dir */
            if (fno.fattrib & AM_DIR) {            /* Directory */
            	myprintf("   <DIR>   %s\n", fno.fname);
            	strcpy(list_folder[ndir],fno.fname);
                ndir++;
            }
            else {                               /* File */
            	myprintf("%10u %s\n", fno.fsize, fno.fname);
                nfile++;
            }
        }
//      Bubble Sort
    	for(int i = 0; i < ndir; ++i){
    		for(int j = i+1; j < ndir; ++j){
    			if(strcmp(list_folder[i], list_folder[j]) > 0){
    				swap(list_folder[j], list_folder[i]);
    			}
    		}
    	}

        f_closedir(&dir);
        myprintf("%d dirs, %d files.\n", ndir, nfile);
    } else {
    	myprintf("Failed to open \"%s\". (%u)\n", path, res);
    }
    return res;
}


