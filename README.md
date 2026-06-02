# CloudBrowser
A browser for folders connected to protondrive

### Requirements
Read [requirements.md](requirements.md)

### Installation
Clone repo
```bash
git clone https://github.com/caraortizmah/CloudBrowser.git
```
and inside repo
```bash
cd CloudBrowser/
qmake
make
./FolderBrowser
```


### Configuration

```bash
mkdir -p ~/.config/folderbrowser
cat > ~/.config/folderbrowser/config.json << 'EOF'
{
    "folder_path": "/mnt/rclone/your_mount_point"
}
EOF
```