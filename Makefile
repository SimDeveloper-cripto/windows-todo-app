IMAGE_NAME     = raylib-todo-app
CONTAINER_NAME = todo-container

ifeq ($(OS),Windows_NT)
	DB_VOLUME = $(shell cd)
else
	DB_VOLUME = $(shell pwd)
endif

build:
	@echo "--- Compiling Docker Image '$(IMAGE_NAME)' ---"
	docker build -t $(IMAGE_NAME) .

run:
	@echo "--- Docker is now running [TODO App] ---"
	docker run --rm -it \
        -e DISPLAY=host.docker.internal:0.0 \
        -v $(DB_VOLUME):/app \
        --name $(CONTAINER_NAME) \
        $(IMAGE_NAME) bash

clean:
	-docker stop $(CONTAINER_NAME) 2>/dev/null || true
	-docker rmi $(IMAGE_NAME) 2>/dev/null || true

clean_db:
	rm -f todo_list.db

.PHONY: build run clean clean_db