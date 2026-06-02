# CloudBrowser
A browser for folders connected to protondrive

### Requirements
Read [requirements.md](requirements.md)

### Configuration

You need a configuration file (json) and the list of folders (txt):
`~/.config/folderbrowser/config.json` and `~/.config/folderbrowser/folders.txt`

Two options:
#### 1. Use templates
Go to `templates/` and check files there

#### 2. Create them

```bash
mkdir -p ~/.config/folderbrowser
cat > ~/.config/folderbrowser/config.json << 'EOF'
{
    "folder_path": "/home/user/Documents",
    "intercept_folders": ["SpecialFolder", "RcloneMount", "VirtualDir"],
    "custom_list_file": "/home/user/.config/folderbrowser/folders.txt"
}
```

and

```bash
ls $list_path >> ~/.config/folderbrowser/folders.txt
```

They should look like the examples in `templates/`

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