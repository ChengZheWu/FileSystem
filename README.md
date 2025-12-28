1. 編譯
```
make
```
2. 格式化硬碟 myfs.img
```
./mkfs
```
3. 執行掛載
```
./myfs -f mnt
```
4. 建立檔案
```
# 分配 Inode 1 (檢查 Log 是否顯示 Inode 1 allocated)
touch mnt/a.txt
```
5. 寫入資料
```
# 分配 Data Block 0 (檢查 Log 是否顯示 Block allocated)
echo "Hello" > mnt/a.txt
```
6. 讀取資料
```
cat mnt/a.txt
```