1. 編譯
```
make
```
2. 確保 mnt, storage 存在
```
mkdir -p mnt storage

# 先隨便塞個檔案進去測試
echo "I am real file" > storage/test.txt
```
3. 執行掛載
```
./myfs -f mnt
```
4. 驗證
```
ls -l mnt
# 應該會看到 test.txt！

cat mnt/test.txt
# 內容應該是 "I am real file"

echo "FUSE is cool" > mnt/newfile.txt
# 去檢查 storage/newfile.txt，看看檔案是不是真的出現了？
```