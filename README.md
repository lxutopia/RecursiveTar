# Tar parsing tool
This tool is built for efficient reading of nested tar archives without the need of unpacking them first. What makes this tool unique is the fact that it can read multiple levels of tar files inside one another in.

## Usage
> peektar &lt;path To Archive> [ &lt;Path Within Archive> ]

The first path is the path of the .tar archive file to be read. If only the first argument is specified the content of the archive will be listed. If the second path (the path within the archive) is specified the file will be returned to stdout as binary.

## Limitations
The tool does not support compressed tar files
TODO nåt.

## Support
Create a issue for support: https://github.com/Sydarkivera/RecursiveTar/issues

## License
This project is licensed under the terms of the GNU (General Public License)
