#include <assert.h>
#include <string.h>
#include <errno.h>

#include "fat16.h"

// 忘记写到fat16.h 里的函数定义
extern FAT16* get_fat16_ins();

/**
 * @brief 请勿修改该函数。
 * 该函数用于修复5月13日发布的simple_fat16_part1.c中RootOffset和DataOffset的计算错误。
 * 如果你在Part1中也使用了以下字段或函数：
 *     fat16_ins->RootOffset
 *     fat16_ins->DataOffset
 *     find_root函数的offset_dir输出参数
 * 请手动修改pre_init_fat16函数定义中fat16_ins->RootOffset的计算，如下：
 * 正确的计算：
 *      fat16_ins->RootOffset = fat16_ins->FatOffset + fat16_ins->FatSize * fat16_ins->Bpb.BPB_NumFATS;
 * 错误的计算（5月13日发布的simple_fat16_part1.c中的版本）：
 *   // fat16_ins->RootOffset = fat16_ins->FatOffset * fat16_ins->FatSize * fat16_ins->Bpb.BPB_NumFATS;
 * 即将RootOffset计算中第一个乘号改为加号。
 * @return FAT16* 修复计算并返回文件系统指针
 */
FAT16* get_fat16_ins_fix() {
  FAT16 *fat16_ins = get_fat16_ins();
  fat16_ins->FatOffset =  fat16_ins->Bpb.BPB_RsvdSecCnt * fat16_ins->Bpb.BPB_BytsPerSec;
  fat16_ins->FatSize = fat16_ins->Bpb.BPB_BytsPerSec * fat16_ins->Bpb.BPB_FATSz16;
  fat16_ins->RootOffset = fat16_ins->FatOffset + fat16_ins->FatSize * fat16_ins->Bpb.BPB_NumFATS;
  fat16_ins->ClusterSize = fat16_ins->Bpb.BPB_BytsPerSec * fat16_ins->Bpb.BPB_SecPerClus;
  fat16_ins->DataOffset = fat16_ins->RootOffset + fat16_ins->Bpb.BPB_RootEntCnt * BYTES_PER_DIR;
  return fat16_ins;
}

/**
 * @brief 簇号是否是合法的（表示正在使用的）数据簇号（在CLUSTER_MIN和CLUSTER_MAX之间）
 * 
 * @param cluster_num 簇号
 * @return int 合法返回1 非法返回0   
 */
int is_cluster_inuse(uint16_t cluster_num)
{
  return CLUSTER_MIN <= cluster_num && cluster_num <= CLUSTER_MAX;
}

/**
 * @brief 将data写入簇号为clusterN的簇对应的FAT表项，注意要对文件系统中所有FAT表(两个表)都进行相同的写入。
 * 
 * @param fat16_ins 文件系统指针
 * @param clusterN  要写入表项的簇号
 * @param data      要写入表项的数据，如下一个簇号，CLUSTER_END（文件末尾），或者0（释放该簇）等等
 * @return int      成功返回0
 */
int write_fat_entry(FAT16 *fat16_ins, WORD clusterN, WORD data) {
  // Hint: 这个函数逻辑与fat_entry_by_cluster函数类似，但这个函数需要修改对应值并写回FAT表中
  BYTE sector_buffer[BYTES_PER_SECTOR];
  /** TODO: 计算下列值，当然，你也可以不使用这些变量*/
  if(is_cluster_inuse(clusterN)  == 0){
    return -EINVAL;//越界
  }
  uint FirstFatSecNum;  // 第一个FAT表开始的扇区号
  uint ClusterOffset;   // clusterN这个簇对应的表项，在每个FAT表项的哪个偏移量
  uint ClusterSec;      // clusterN这个簇对应的表项，在每个FAT表中的第几个扇区（Hint: 这个值与ClusterSec的关系是？）
  uint SecOffset;       // clusterN这个簇对应的表项，在所在扇区的哪个偏移量（Hint: 这个值与ClusterSec的关系是？）
  /*** BEGIN ***/
  FirstFatSecNum = fat16_ins->Bpb.BPB_RsvdSecCnt;
  ClusterOffset = clusterN*2;
  ClusterSec = FirstFatSecNum + (ClusterOffset/fat16_ins->Bpb.BPB_BytsPerSec);
  SecOffset = ClusterOffset % fat16_ins->Bpb.BPB_BytsPerSec;

  FILE *fd = fat16_ins->fd;
  /*** END ***/
  // Hint: 对系统中每个FAT表都进行写入
  for(uint i=0; i<2; i++) {
    /*** BEGIN ***/
    // Hint: 计算出当前要写入的FAT表扇区号
    // Hint: 读扇区，在正确偏移量将值修改为data，写回扇区
    sector_read(fd, ClusterSec, sector_buffer);

    memcpy(sector_buffer+SecOffset, &data, sizeof(WORD));

    sector_write(fd, ClusterSec, sector_buffer);

    ClusterSec += fat16_ins->Bpb.BPB_FATSz16;//FAT2

    /*** END ***/
  }
  return 0;
}

/**
 * @brief 分配n个空闲簇，分配过程中将n个簇通过FAT表项连在一起，然后返回第一个簇的簇号。
 *        最后一个簇的FAT表项将会指向0xFFFF（即文件中止）。此处有清除簇内容的操作
 * @param fat16_ins 文件系统指针
 * @param n         要分配簇的个数
 * @return WORD 分配的第一个簇，分配失败，将返回CLUSTER_END，若n==0，也将返回CLUSTER_END。
 */
WORD alloc_clusters(FAT16 *fat16_ins, uint32_t n) {
  if (n == 0)
    return CLUSTER_END;
  BYTE sector_buffer[BYTES_PER_SECTOR];

  // Hint: 用于保存找到的n个空闲簇，另外在末尾加上CLUSTER_END，共n+1个簇号
  WORD *clusters = malloc((n + 1) * sizeof(WORD));
  WORD fat_entry;
  uint allocated = 0; // 已找到的空闲簇个数
  uint FatSecNum = fat16_ins->Bpb.BPB_RsvdSecCnt ;
  /** TODO: 扫描FAT表，找到n个空闲的簇，存入cluster数组。注意此时不需要修改对应的FAT表项 **/
  /*** BEGIN ***/
  for (size_t i = 0; i < fat16_ins->Bpb.BPB_FATSz16 && allocated < n; i++){
    sector_read(fat16_ins->fd, FatSecNum + i, sector_buffer);
    for (size_t offset = 0; offset < fat16_ins->Bpb.BPB_BytsPerSec && allocated < n; offset += 2){
      if(i == 0 && offset < 4){
        offset = 4;
        continue;
      }
      memcpy(&fat_entry, sector_buffer + offset, 2);//表项有两个字节长
      if(fat_entry == CLUSTER_FREE){
        clusters[allocated] = (i * fat16_ins->Bpb.BPB_BytsPerSec + offset)/2 ;
        allocated++; 
      }
    }
  }

  /*** END ***/

  if(allocated != n) {  // 找不到n个簇，分配失败
    free(clusters);
    return CLUSTER_END;
  }

  // Hint: 找到了n个空闲簇，将CLUSTER_END加至末尾。
  clusters[n] = CLUSTER_END;
  DWORD secnum;

  /** TODO: 修改clusters中存储的N个簇对应的FAT表项，将每个簇与下一个簇连接在一起。同时清零每一个新分配的簇。**/
  /*** BEGIN ***/
  
  memset(sector_buffer, 0, BYTES_PER_SECTOR);
  for (size_t i = 0; i < n; i++){
    write_fat_entry(fat16_ins, clusters[i], clusters[i+1]);
    secnum = ((clusters[i] - 2) * fat16_ins->Bpb.BPB_SecPerClus) + fat16_ins->FirstDataSector;
    for (size_t j = 0; j < fat16_ins->Bpb.BPB_SecPerClus; j++){
      sector_write(fat16_ins->fd, secnum + j, sector_buffer);
    }   
  }

  /*** END ***/

  // 返回首个分配的簇
  WORD first_cluster = clusters[0];
  free(clusters);
  return first_cluster;
}


// ------------------TASK3: 创建/删除文件夹-----------------------------------

/**
 * @brief 创建path对应的文件夹
 * 
 * @param path 创建的文件夹路径
 * @param mode 文件模式，本次实验可忽略，默认都为普通文件夹
 * @return int 成功:0， 失败: POSIX错误代码的负值
 */
int fat16_mkdir(const char *path, mode_t mode) {
  /* Gets volume data supplied in the context during the fat16_init function */
  FAT16 *fat16_ins = get_fat16_ins_fix();

 
  /** TODO: 模仿mknod，计算出findFlag, sectorNum和offset的值
   *  你也可以选择不使用这些值，自己定义其它变量。注意本函数前半段和mknod前半段十分类似。
   **/
  /*** BEGIN ***/
  // 查找需要创建文件的父目录路径
  int pathDepth;
  char **paths = path_split((char *)path, &pathDepth);
  char *copyPath = strdup(path);
  const char **orgPaths = (const char **)org_path_split(copyPath);
  char *prtPath = get_prt_path(path, orgPaths, pathDepth);

  BYTE sector_buffer[BYTES_PER_SECTOR];
  
  int findFlag = 0;       // 是否找到空闲的目录项
  DWORD sectorNum = 0;      // 找到的空闲目录项所在扇区号
  off_t offset_sector = 0,  // 找到的空闲目录项在扇区中的偏移量
        offset_dir;
  int RootDirCnt = 1, 
      DirSecCnt = 1;
  WORD ClusterN, 
       FatClusEntryVal;
  DWORD FirstSectorofCluster;
  DIR_ENTRY Dir, prtDir;
  /* If parent directory is root */
  if (strcmp(prtPath, "/") == 0){
    /**
     * 遍历根目录下的目录项
     * 如果发现有同名文件，则直接中断，findFlag=0
     * 找到可用的空闲目录项，即0x00位移处为0x00或者0xe5的项, findFlag=1
     * 并记录对应的sectorNum, offset等可能会用得到的信息
     **/    
    /*** BEGIN ***/
    sector_read(fat16_ins->fd, fat16_ins->FirstRootDirSecNum, sector_buffer);
    prtDir.DIR_FstClusLO = fat16_ins->FirstRootDirSecNum;
    for (size_t i= 1; i <= fat16_ins->Bpb.BPB_RootEntCnt ; i++){
      memcpy(&Dir, &sector_buffer[((i - 1) * BYTES_PER_DIR) % BYTES_PER_SECTOR], BYTES_PER_DIR);//取buffer中一个目录条目大小（32B）的内容
      if (strncmp((const char *)Dir.DIR_Name, paths[0], 11) == 0 ){
        findFlag=0;
        break;
      }
      if(findFlag == 0 ){
        if ( Dir.DIR_Name[0] == 0x00 || Dir.DIR_Name[0] == 0xe5) {
          findFlag=1;
          sectorNum = fat16_ins->FirstRootDirSecNum + RootDirCnt-1;
          offset_sector = ((i - 1) * BYTES_PER_DIR) % BYTES_PER_SECTOR;
          
        }
      }
      if(Dir.DIR_Name[0] == 0x00)
        break;

      if(i % 16 == 0 && i != fat16_ins->Bpb.BPB_RootEntCnt) {
        sector_read(fat16_ins->fd, fat16_ins->FirstRootDirSecNum + RootDirCnt, sector_buffer);//读取目录区中下一个扇区(32*16=512)
        RootDirCnt++;
      }
    }
    

    /*** END ***/
  }
  /* Else if parent directory is sub-directory */
  else
  {
    /**
     * 遍历根目录下的目录项
     * 如果发现有同名文件，则直接中断，findFlag=0
     * 找到可用的空闲目录项，即0x00位移处为0x00或者0xe5的项, findFlag=1
     * 并记录对应的sectorNum, offset等可能会用得到的信息
     **/    
    if(find_root(fat16_ins, &prtDir, prtPath, &offset_dir)==1){
      return -ENOENT;
    } 
    ClusterN = prtDir.DIR_FstClusLO;
    first_sector_by_cluster(fat16_ins, ClusterN, &FatClusEntryVal, &FirstSectorofCluster, sector_buffer);//找该簇号对应的FAT表项FatClusEntryVal,第一个扇区号FirstSectorofCluster,第一个扇区的内容
    
    if(prtDir.DIR_Attr == ATTR_ARCHIVE){
        return  ENOTDIR;
    }
    
    for(size_t i = 1;Dir.DIR_Name[0] != 0x00 ; i++){
      memcpy(&Dir, &sector_buffer[((i - 1) * BYTES_PER_DIR) % BYTES_PER_SECTOR], BYTES_PER_DIR);//取buffer中一个目录条目大小（32B）的内容到Dir中
      
      if (strncmp((const char *)Dir.DIR_Name, paths[pathDepth - 1], 11) == 0 ){
        findFlag=0;
        break;
      }
      if(findFlag == 0){
        if ( Dir.DIR_Name[0] == 0x00 || Dir.DIR_Name[0] == 0xe5) {
          findFlag=1;
          sectorNum = FirstSectorofCluster + DirSecCnt - 1;
          offset_sector = ((i - 1) * BYTES_PER_DIR) % BYTES_PER_SECTOR;
        }
      }
      if(Dir.DIR_Name[0] == 0x00)
        break;
      // 当前扇区的所有目录项已经读完。
      if (i % 16 == 0) {
        // 如果当前簇还有未读的扇区
        if (DirSecCnt < fat16_ins->Bpb.BPB_SecPerClus) {
          // 将下一个扇区的数据读入sector_buffer
          sector_read(fat16_ins->fd, FirstSectorofCluster + DirSecCnt, sector_buffer);//当前扇区读完了，读该簇内的下一个扇区
          DirSecCnt++;
        }else{
          if(FatClusEntryVal == CLUSTER_END){//子目录的簇用完了
            break;
          }
          //读取下一个簇（即簇号为FatClusEntryVal的簇）的第一个扇区
          ClusterN = FatClusEntryVal;
          first_sector_by_cluster(fat16_ins, ClusterN, &FatClusEntryVal, &FirstSectorofCluster, sector_buffer);//重新赋值给FAT表项，簇内第一个扇区号
          i = 0;
          DirSecCnt = 1;
        }

      }

    }
  }


  /*** END ***/

  /** TODO: 在父目录的目录项中添加新建的目录。
   *        同时，为新目录分配一个簇，并在这个簇中创建两个目录项，分别指向. 和 .. 。
   *        目录的文件大小设置为0即可。
   *  HINT: 使用正确参数调用dir_entry_create来创建上述三个目录项。
   **/
  if (findFlag == 1) {
    /*** BEGIN ***/
    //创建文件名
    char pathFormatted[MAX_SHORT_NAME_LEN]; 
    pathFormatted[0] = '.';
    for (size_t i = 1; i < 11; i++){
      pathFormatted[i] = ' ';
    }
    pathFormatted[11] = '\0';

    ClusterN = alloc_clusters(fat16_ins, 1);
    if(ClusterN != CLUSTER_END){
      //父目录中条目创建
      dir_entry_create(fat16_ins, sectorNum, offset_sector, paths[pathDepth - 1], ATTR_DIRECTORY, ClusterN, 0);
      //新目录条目内容创建
      FirstSectorofCluster = ((ClusterN - 2) * fat16_ins->Bpb.BPB_SecPerClus) + fat16_ins->FirstDataSector;
      dir_entry_create(fat16_ins, FirstSectorofCluster, 0, pathFormatted, ATTR_DIRECTORY, ClusterN, 0);
      pathFormatted[1] = '.';
      dir_entry_create(fat16_ins, FirstSectorofCluster, BYTES_PER_DIR, pathFormatted, ATTR_DIRECTORY, prtDir.DIR_FstClusLO, 0);
    }
    return 0;
    /*** END ***/
  }
  return -ENOENT;
}


/**
 * @brief 删除offset位置的目录项
 * 
 * @param fat16_ins 文件系统指针
 * @param offset    find_root传回的offset_dir值
 */
void dir_entry_delete(FAT16 *fat16_ins, off_t offset) {
  BYTE buffer[BYTES_PER_SECTOR];
  /** TODO: 删除目录项，或者说，将镜像文件offset处的目录项第一个字节设置为0xe5即可。
   *  HINT: offset对应的扇区号和扇区的偏移量是？只需要读取扇区，修改offset处的一个字节，然后将扇区写回即可。
   */
  WORD ClusterN, FatClusEntryVal;
  DWORD secnum;
  off_t offset_sector;
  
  /*** BEGIN ***/
  if(fat16_ins->RootOffset <= offset && offset < fat16_ins->RootOffset + fat16_ins->Bpb.BPB_RootEntCnt * BYTES_PER_DIR){
    secnum = fat16_ins->FirstRootDirSecNum + (offset - fat16_ins->RootOffset)/BYTES_PER_SECTOR;
    offset_sector = (offset - fat16_ins->RootOffset) % BYTES_PER_SECTOR;

  }else if(fat16_ins->DataOffset <= offset){
    ClusterN = (offset - fat16_ins->DataOffset)/fat16_ins->ClusterSize + 2;
    if(!is_cluster_inuse(ClusterN)){
      return ;
    }
    secnum =fat16_ins->FirstDataSector + (offset - fat16_ins->DataOffset) / BYTES_PER_SECTOR;
    offset_sector = (offset - fat16_ins->DataOffset) % BYTES_PER_SECTOR;
  }
  sector_read(fat16_ins->fd, secnum, buffer);
  buffer[offset_sector] = 0xe5;
  sector_write(fat16_ins->fd, secnum, buffer);

  /*** END ***/
}

/**
 * @brief 写入offset位置的目录项
 * 
 * @param fat16_ins 文件系统指针
 * @param offset    find_root传回的offset_dir值
 * @param Dir       要写入的目录项
 */
void dir_entry_write(FAT16 *fat16_ins, off_t offset, const DIR_ENTRY *Dir) {
  BYTE buffer[BYTES_PER_SECTOR];
  // TODO: 修改目录项，和dir_entry_delete完全类似，只是需要将整个Dir写入offset所在的位置。
  WORD ClusterN, FatClusEntryVal;
  DWORD secnum;
  off_t offset_sector;
  
  /*** BEGIN ***/
  if(fat16_ins->RootOffset <= offset && offset< fat16_ins->RootOffset + fat16_ins->Bpb.BPB_RootEntCnt * BYTES_PER_DIR){
    secnum = fat16_ins->FirstRootDirSecNum + (offset - fat16_ins->RootOffset)/BYTES_PER_SECTOR;
    offset_sector = (offset - fat16_ins->RootOffset) % BYTES_PER_SECTOR;

  }else if(fat16_ins->DataOffset <= offset){
    ClusterN = (offset - fat16_ins->DataOffset)/fat16_ins->ClusterSize + 2;
    if(!is_cluster_inuse(ClusterN)){
      return ;
    }
    secnum =fat16_ins->FirstDataSector + (offset - fat16_ins->DataOffset) / BYTES_PER_SECTOR;
    offset_sector = (offset - fat16_ins->DataOffset) % BYTES_PER_SECTOR;
  }  
  sector_read(fat16_ins->fd, secnum, buffer);
  memcpy(buffer + offset_sector, Dir, BYTES_PER_DIR);
  sector_write(fat16_ins->fd, secnum, buffer);
  /*** END ***/
}

/**
 * @brief 删除path对应的文件夹
 * 
 * @param path 要删除的文件夹路径
 * @return int 成功:0， 失败: POSIX错误代码的负值
 */
int fat16_rmdir(const char *path) {
  /* Gets volume data supplied in the context during the fat16_init function */
  FAT16 *fat16_ins = get_fat16_ins_fix();

  if(strcmp(path, "/") == 0) {
    return -EBUSY;  // 无法删除根目录，根目录是挂载点（可参考`man 2 rmdir`）
  }

  DIR_ENTRY Dir, Root;
  DIR_ENTRY curDir;
  // // 查找需要删除文件的父目录路径
  // int pathDepth;
  // char **paths = path_split((char *)path, &pathDepth);
  // char *copyPath = strdup(path);
  // const char **orgPaths = (const char **)org_path_split(copyPath);
  // char *prtPath = get_prt_path(path, orgPaths, pathDepth);

  BYTE sector_buffer[BYTES_PER_SECTOR];
  DWORD sectorNum;
  int i, findFlag = 0,is_empty = 1, RootDirCnt = 1, DirSecCnt = 1;
  WORD FatClusEntryVal, ClusterN;
  DWORD FirstSectorofCluster;
  off_t offset_dir;
  int res = find_root(fat16_ins, &Dir, path, &offset_dir);

  if(res != 0) {
    return -ENOENT; // 路径不存在
  }

  if(Dir.DIR_Attr != ATTR_DIRECTORY) {
    return ENOTDIR; // 路径不是目录
  }
  curDir = Dir;

  /** TODO: 检查目录是否为空，如果目录不为空，直接返回-ENOTEMPTY。
   *        注意空目录也可能有"."和".."两个子目录。
   *  HINT: 这一段和readdir的非根目录部分十分类似。
   *  HINT: 注意忽略DIR_Attr为0x0F的长文件名项(LFN)。
   **/

  /*** BEGIN ***/
  for(i = 1; ; i++){
    memcpy(&Dir, &sector_buffer[((i - 1) * BYTES_PER_DIR) % BYTES_PER_SECTOR], BYTES_PER_DIR);//取buffer中一个目录条目大小（32B）的内容到Dir中

    if(DirSecCnt >1 && i >2){
      if(Dir.DIR_Name[0] == 0x00 ){
        break;
      }
      if( Dir.DIR_Name[0] != 0xe5 && Dir.DIR_Name[0] != 0x0f){
        is_empty = 0;
        break;
      }
    }
    // 当前扇区的所有目录项已经读完。
    if (i % 16 == 0) {
      // 如果当前簇还有未读的扇区
      if (DirSecCnt < fat16_ins->Bpb.BPB_SecPerClus) {
        // 将下一个扇区的数据读入sector_buffer
        sector_read(fat16_ins->fd, FirstSectorofCluster + DirSecCnt, sector_buffer);//当前扇区读完了，读该簇内的下一个扇区
        DirSecCnt++;
      }else{
        if(FatClusEntryVal == CLUSTER_END){//子目录的簇读完了
          is_empty = 1;
          break;
        }
        //读取下一个簇（即簇号为FatClusEntryVal的簇）的第一个扇区
        ClusterN = FatClusEntryVal;
        first_sector_by_cluster(fat16_ins, ClusterN, &FatClusEntryVal, &FirstSectorofCluster, sector_buffer);//重新赋值给FAT表项，簇内第一个扇区号
        i = 0;
        DirSecCnt = 1;
      }
    }
  }
  if(!is_empty){
    return -ENOTEMPTY;
  }

  /*** END ***/

  // 已确认目录项为空，释放目录占用的簇,即将FAT表项置0
  // TODO: 循环调用free_cluster释放对应簇，和unlink类似。
  /*** BEGIN ***/
  ClusterN = curDir.DIR_FstClusLO;
  while (ClusterN != CLUSTER_END) {
    ClusterN = free_cluster(fat16_ins, ClusterN);
  }
  /*** END ***/

  // TODO: 删除父目录中的目录项
  // HINT: 如果你正确实现了dir_entry_delete，这里只需要一行代码调用它即可
  //       你也可以使用你在unlink使用的方法。
  /*** BEGIN ***/
  dir_entry_delete(fat16_ins, offset_dir);

  /*** END ***/

  return 0;
}


// ------------------TASK4: 写文件-----------------------------------

/**
 * @brief 将data中的数据写入编号为clusterN的簇的offset位置。
 *        注意size+offset <= 簇大小,以簇为力度写数据
 * 
 * @param fat16_ins 文件系统指针
 * @param clusterN  要写入数据的块号
 * @param data      要写入的数据
 * @param size      要写入数据的大小（字节）
 * @param offset    要写入簇的偏移量
 * @return size_t   成功写入的字节数
 */
size_t write_to_cluster_at_offset(FAT16 *fat16_ins, WORD clusterN, off_t offset, const BYTE* data, size_t size) {
  assert(offset + size <= fat16_ins->ClusterSize);  // offset + size 必须小于簇大小
  BYTE sector_buffer[BYTES_PER_SECTOR];
  /** TODO: 将数据写入簇对应的偏移量上。
   *        你需要找到第一个需要写入的扇区，和要写入的偏移量，然后依次写入后续每个扇区，直到所有数据都写入完成。
   *        注意，offset对应的首个扇区和offset+size对应的最后一个扇区都可能只需要写入一部分。
   *        所以应该先将扇区读出，修改要写入的部分，再写回整个扇区。
   */
  /*** BEGIN ***/
  if(clusterN == CLUSTER_END)
    return 0;

  off_t sector_offset = 0, data_offset = 0, beginBytes=0;
  int  i = 0,
       DirSecCnt = 0; 
  DWORD FirstSectorofCluster;
  WORD FatClusEntryVal;
  while( (beginBytes + fat16_ins->Bpb.BPB_BytsPerSec) <= offset && DirSecCnt < fat16_ins->Bpb.BPB_SecPerClus){//以扇区为力度查找
    beginBytes += fat16_ins->Bpb.BPB_BytsPerSec;
    DirSecCnt ++;
  }

  if(DirSecCnt == fat16_ins->Bpb.BPB_SecPerClus){
    return 0;
  }

  first_sector_by_cluster(fat16_ins, clusterN, &FatClusEntryVal, &FirstSectorofCluster, sector_buffer);
  sector_read(fat16_ins->fd, FirstSectorofCluster + DirSecCnt, sector_buffer);
  sector_offset = offset - beginBytes;
  for ( data_offset = 0; data_offset < size && DirSecCnt < fat16_ins->Bpb.BPB_SecPerClus; data_offset++){
    if(sector_offset >= fat16_ins->Bpb.BPB_BytsPerSec){
      sector_offset = 0;
      sector_write(fat16_ins->fd, FirstSectorofCluster + DirSecCnt, sector_buffer);
      DirSecCnt++;//扇区数++
      sector_read(fat16_ins->fd, FirstSectorofCluster + DirSecCnt, sector_buffer);//读下一个扇区
    }
    sector_buffer[sector_offset++] = data[data_offset];
  }
  sector_write(fat16_ins->fd, FirstSectorofCluster + DirSecCnt, sector_buffer);
  /*** END ***/
  return size;
}


/**
 * @brief 查找文件最末尾的一个簇，同时计算文件当前簇数，如果文件没有任何簇，返回CLUSTER_END。
 * 
 * @param fat16_ins 文件系统指针 
 * @param Dir       文件的目录项
 * @param count     输出参数，当为NULL时忽略该参数，否则设置为文件当前簇的数量
 * @return WORD     文件最后一个簇的编号
 */
WORD file_last_cluster(FAT16 *fat16_ins, DIR_ENTRY *Dir, int64_t *count) {
  
  int64_t cnt = 0;        // 文件拥有的簇数量
  WORD cur = CLUSTER_END; // 最后一个被文件使用的簇号
  // TODO: 找到Dir对应的文件的最后一个簇编号，并将该文件当前簇的数目填充入count
  // HINT: 可能用到的函数：is_cluster_inuse和fat_entry_by_cluster函数。
  /*** BEGIN ***/
  WORD clusterN = Dir->DIR_FstClusLO;
  WORD next_clus;
  if(clusterN != CLUSTER_END){
    cnt++;
    while ((next_clus = fat_entry_by_cluster(fat16_ins, clusterN) ) != CLUSTER_END){
      cnt++;  
      clusterN = next_clus;
    }
    cur = clusterN;
  }

  /*** END ***/
  if(count != NULL) { // 如果count为NULL，不填充count
    *count = cnt;
  }
  return cur;
}

/**
 * @brief 为Dir指向的文件新分配count个簇，并将其连接在文件末尾，保证新分配的簇全部以0填充。此时真正修改清空了簇（data）区的内容
 *        注意，如果文件当前没有任何簇，本函数应该修改Dir->DIR_FstClusLO值，使其指向第一个簇。
 * 
 * @param fat16_ins     文件系统指针
 * @param Dir           要分配新簇的文件的目录项
 * @param last_cluster  file_last_cluster的返回值，当前该文件的最后一个簇簇号。
 * @param count         要新分配的簇数量
 * @return int 成功返回分配前原文件最后一个簇簇号，失败返回POSIX错误代码的负值
 */
int file_new_cluster(FAT16 *fat16_ins, DIR_ENTRY *Dir, WORD last_cluster, DWORD count)
{
  /** TODO: 先用alloc_clusters分配count个簇。
   *        然后若原文件本身有至少一个簇，修改原文件最后一个簇的FAT表项，使其与新分配的簇连接。
   *        否则修改Dir->DIR_FstClusLO值，使其指向第一个簇。
   */
  /*** BEGIN ***/

  WORD ClusterN, FstAllocClus;
  FstAllocClus = alloc_clusters(fat16_ins, count );
  if(FstAllocClus == CLUSTER_END){
    return -ENOENT;
  }
  
  if(last_cluster == CLUSTER_END){
    Dir->DIR_FstClusLO = FstAllocClus;
  }else{
    write_fat_entry(fat16_ins, last_cluster, FstAllocClus);
  }

  /*** END ***/
  return last_cluster;
}

/**
 * @brief 在文件offset的位置写入buff中的数据，数据长度为length。
 * 
 * @param fat16_ins   文件系统执政
 * @param Dir         要写入的文件目录项
 * @param offset_dir  find_root返回的offset_dir值
 * @param buff        要写入的数据
 * @param offset      文件要写入的位置
 * @param length      要写入的数据长度（字节）
 * @return int        成功时返回成功写入数据的字节数，失败时返回POSIX错误代码的负值
 */
int write_file(FAT16 *fat16_ins, DIR_ENTRY *Dir, off_t offset_dir, const void *buff, off_t offset, size_t length)
{

  if (length == 0)
    return 0;

  if (offset + length < offset) // 溢出了
    return -EINVAL;

  if(offset > Dir->DIR_FileSize)
    return -EFAULT;//地址错误

  /** TODO: 通过offset和length，判断文件是否修改文件大小，以及是否需要分配新簇，并正确修改大小和分配簇。
   *  HINT: 可能用到的函数：file_last_cluster, file_new_cluster等
   */
  /*** BEGIN ***/
  WORD LastCluster, clusterN, nextCluster ;
  int64_t curClusCnt = 0, DataClusCnt = 0, newClusCnt, BeginClusCnt;
  off_t offset_cluster, offset_buffer;
  size_t data_size;
  newClusCnt = ( offset + length ) / fat16_ins->ClusterSize + 1;
  BeginClusCnt = offset / fat16_ins->ClusterSize ;
  offset_cluster = offset % fat16_ins->ClusterSize ;
  if(BeginClusCnt+1 < newClusCnt )//跨簇
    data_size = fat16_ins->ClusterSize - offset_cluster;
  else
    data_size = length;

  LastCluster = file_last_cluster(fat16_ins, Dir, &curClusCnt);
  if(curClusCnt < newClusCnt){
    file_new_cluster(fat16_ins, Dir, LastCluster, newClusCnt - curClusCnt);
    Dir->DIR_FileSize = offset + length;
  }else if(offset + length > Dir->DIR_FileSize){
    Dir->DIR_FileSize = offset + length ;
  }

  /*** END ***/

  /** TODO: 和read类似，找到对应的偏移，并写入数据。
   *  HINT: 如果你正确实现了write_to_cluster_at_offset，此处逻辑会简单很多。
   */
  /*** BEGIN ***/
  // HINT: 记得把修改过的Dir写回目录项（如果你之前没有写回）
  dir_entry_write(fat16_ins, offset_dir, Dir);

  clusterN = Dir->DIR_FstClusLO;
  nextCluster = fat_entry_by_cluster(fat16_ins, clusterN);
  curClusCnt = 0;
  while (curClusCnt  < BeginClusCnt){
    curClusCnt++;
    clusterN = nextCluster;
    nextCluster = fat_entry_by_cluster(fat16_ins, clusterN);
  }

  offset_buffer = 0;
  while (length > 0 && clusterN != CLUSTER_END){//以簇为力度写入
    write_to_cluster_at_offset(fat16_ins, clusterN, offset_cluster, (const BYTE*)buff + offset_buffer, data_size);
    length -= data_size;
    offset_buffer += data_size;
    if(length / fat16_ins->ClusterSize != 0){
      data_size = fat16_ins->ClusterSize;
    }else{
      data_size = length % fat16_ins->ClusterSize;
    }
    clusterN = nextCluster;
    nextCluster = fat_entry_by_cluster(fat16_ins, clusterN);
    offset_cluster = 0;
  }
  if(length == 0){
    return offset_buffer;
  }
  /*** END ***/
  return -EIO;
}

/**
 * @brief 将长度为size的数据data写入path对应的文件的offset位置。注意当写入数据量超过文件本身大小时，
 *        需要扩展文件的大小，必要时需要分配新的簇。
 * 
 * @param path    要写入的文件的路径
 * @param data    要写入的数据
 * @param size    要写入数据的长度
 * @param offset  文件中要写入数据的偏移量（字节）
 * @param fi      本次实验可忽略该参数
 * @return int    成功返回写入的字节数，失败返回POSIX错误代码的负值。
 */
int fat16_write(const char *path, const char *data, size_t size, off_t offset,
                struct fuse_file_info *fi)
{
  FAT16 *fat16_ins = get_fat16_ins_fix();
  /** TODO: 大部分工作都在write_file里完成了，这里调用find_root获得目录项，然后调用write_file即可
   */
  /*** BEGIN ***/
  DIR_ENTRY Dir;
  off_t offset_dir;
  int res = find_root(fat16_ins, &Dir, path, &offset_dir);
  if(res != 0){
    return -ENOENT; // 路径不存在
  }
  return write_file(fat16_ins, &Dir, offset_dir, data, offset, size);

  /*** END ***/
  return 0;
}

/**
 * @brief 将path对应的文件大小改为size，注意size可以大于小于或等于原文件大小。
 *        若size大于原文件大小，需要将拓展的部分全部置为0，如有需要，需要分配新簇。
 *        若size小于原文件大小，将从末尾截断文件，若有簇不再被使用，应该释放对应的簇。
 *        若size等于原文件大小，什么都不需要做。
 * 
 * @param path 需要更改大小的文件路径 
 * @param size 新的文件大小
 * @return int 成功返回0，失败返回POSIX错误代码的负值。
 */
int fat16_truncate(const char *path, off_t size)
{
  /* Gets volume data supplied in the context during the fat16_init function */
  FAT16 *fat16_ins = get_fat16_ins_fix();

  /* Searches for the given path */
  DIR_ENTRY Dir;
  off_t offset_dir, offset_cluster, offset_buff;
  size_t data_size;
  find_root(fat16_ins, &Dir, path, &offset_dir);

  // 当前文件已有簇的数量，以及截断或增长后，文件所需的簇数量。
  int64_t cur_cluster_count, file_cluster_count;
  WORD last_cluster = file_last_cluster(fat16_ins, &Dir, &cur_cluster_count);
  WORD clusterN, nextCluster;
  int64_t count = 0;
  int64_t new_cluster_count = (size + fat16_ins->ClusterSize - 1) / fat16_ins->ClusterSize;

  DWORD new_size = size;
  DWORD old_size = Dir.DIR_FileSize;
  
  BYTE *buff = malloc(fat16_ins->ClusterSize * sizeof(BYTE));
  memset(buff, 0, fat16_ins->ClusterSize);

  file_cluster_count = old_size / fat16_ins->ClusterSize + 1;
  offset_cluster = old_size % fat16_ins->ClusterSize;

  Dir.DIR_FileSize = new_size;

  if (old_size == new_size){
    return 0;
  } else if (old_size < new_size) {
    /** TODO: 增大文件大小，注意是否需要分配新簇，以及往新分配的空间填充0等 **/
    /*** BEGIN ***/
    if(new_cluster_count > cur_cluster_count){
      file_new_cluster(fat16_ins, &Dir, last_cluster, new_cluster_count - cur_cluster_count);
    }
    if(cur_cluster_count > 0){
      if(file_cluster_count < cur_cluster_count){
        data_size = fat16_ins->ClusterSize - offset_cluster;
        clusterN = Dir.DIR_FstClusLO;
        nextCluster = fat_entry_by_cluster(fat16_ins, clusterN);
        count = 1;
        while (count < file_cluster_count)  {
          count++;
          clusterN = nextCluster; 
          nextCluster = fat_entry_by_cluster(fat16_ins, clusterN);
        }
        while (count < cur_cluster_count ){
          write_to_cluster_at_offset(fat16_ins, clusterN, offset_cluster, (const BYTE*)buff, data_size);
          data_size = fat16_ins->ClusterSize;
          count++;
          clusterN = nextCluster;
          nextCluster = fat_entry_by_cluster(fat16_ins, clusterN);
          offset_cluster = 0;
        }         
      }
      data_size = fat16_ins->ClusterSize - offset_cluster;
      write_to_cluster_at_offset(fat16_ins, last_cluster, offset_cluster, (const BYTE*)buff, data_size);
    }     
    /*** END ***/
  } else {  // 截断文件
    /** TODO: 截断文件，注意是否需要释放簇等 **/
    /*** BEGIN ***/
    if(new_cluster_count < cur_cluster_count){
      clusterN = Dir.DIR_FstClusLO;
      nextCluster = fat_entry_by_cluster(fat16_ins, clusterN);
      count=1;
      while (count < new_cluster_count){
        count++;
        clusterN = nextCluster; 
        nextCluster = fat_entry_by_cluster(fat16_ins, clusterN);
      }
      write_fat_entry(fat16_ins, clusterN, CLUSTER_END);
      clusterN = nextCluster;
      while(clusterN != CLUSTER_END){
        clusterN = free_cluster(fat16_ins, clusterN);
      } 
    }    

    /*** END ***/
  }
  dir_entry_write(fat16_ins, offset_dir, &Dir);
  free(buff);
  return 0;
}
