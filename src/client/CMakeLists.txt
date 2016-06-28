# Source files
set(client_source_files
	client.c
	client.h
)

# Generate the executable from the source files
add_executable(client ${client_source_files})

# Necessary libraries
target_link_libraries(client util)
target_link_libraries(client pthread)
