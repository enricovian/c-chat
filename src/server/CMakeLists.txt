# Source files
set(server_source_files
	clientlist.c
	clientlist.h
	server.c
	server.h
)

# Generate the executable from the source files
add_executable(server ${server_source_files})

# Necessary libraries
target_link_libraries(server util)
target_link_libraries(server pthread)
