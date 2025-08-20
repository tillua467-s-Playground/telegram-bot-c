# Telegram Bot C Library

This is a lightweight C library to interact with the Telegram Bot API.
It supports basic messaging features like reading updates, sending messages/documents, deleting, and editing messages.
It's perfect for minimal bots written in C.

---

## 🛠️ Requirements

Before you begin, make sure you have the following:

- GCC compiler
- [`libcurl`](https://curl.se/libcurl/) installed
- [`cJSON`](https://github.com/DaveGamble/cJSON) installed
- Basic knowledge of C programming
- Telegram Bot Token

You will also need to compile the library as a **static `.a` file** (optional for reusability).

---

## 📁 Files

| File             | Description                      |
|------------------|----------------------------------|
| `tgbot.c`        | Source file with all bot logic   |
| `tgbot.h`        | Header file for declarations     |
| `README.md`      | This file                        |
| `last_update.txt`| Used for tracking offset         |

---

## Setting up the bot

### 1. Start by Adding Your Bot Token

In `tgbot.c`, replace the placeholder with your bot’s API token.

```c
char* BOT_API = "YOUR_TELEGRAM_BOT_TOKEN";
```

---

## ⚙️ Functions

### Helper Functions

These are internal utilities to support the main bot functions.

```c
size_t callback(char *ptr, size_t size, size_t nmemb, void *userdata);
void last_up_add();
void last_update_read();
char* latest_file(const char* file_path);
```

- `callback()` → Used internally for handling libcurl responses.
- `last_up_add()` / `last_update_read()` → Save and load last update offset from `last_update.txt`.
- `latest_file(path)` → Returns the latest file from a directory (useful for sending recent downloads).

---

### Main Bot Functions

```c
updateData *get_updates();
void get_document(char *file_id);
int send_message(long long *CID, const char *message, const int reply, long long reply_id);
void send_document(long long *CID, const char *file_name, const int reply, const int reply_id);
void delete_message(long long *CID, const int bot_msgid, long long *group_chat_id);
void edit_message(long long *CID, const int edit_msg_id, const char *message);
```

---

## Receiving Updates
The Update data is actually struct that contain
```c
typedef struct updateData {
    char *chat_Result; // chat message initially "chat"
    long long chat_id; // chat id initially -1
    long long group_chat_id; // group chat id(if it exist) initially -1
    int reply_id;  // reply message id, initially -1
    char *file_id; // file id(if file was sent) initially "file_id=NULL"
} updateData;
```
You can receiving updates by adding this line at the top of your code
```c
updateData *updateData = get_updates();
```
and access them as struct
```c
updateData->chat_Result // or this updateData.chat_Result if u prefer "." (dot)
```

All of the data are

- `chat_Result` → The user's message
- `chat_id` → The chat id
- `group_chat_id` → The group chat id (if any)
- `reply_id` → The message id to reply to
- `file_id` → To download a file by using the get_document() function

---

## Getting Files

When someone sends a file get_updates() function will capture a unique file id with that you can download any files
the function get_document() need a file id inorder to download file
```c
void get_document(char *file_id);
```

start by checking if file id exist and then fetch it-
```c
if(userdata->file_id)
    get_document(userdata->file_id);
```
Keep in mind that when a file is fetched, it will be saved in its respective folder based on its type. For example, photos will be saved in the Photos folder

## Sending Messages

To send a message, use:

```c
int send_message(long long *CID, const char *message, const int reply, long long reply_id);
```

Parameters:

- `CID` → Pointer to chat id
- `message` → Message text to send
- `reply` → 1 to reply to a message, 0 otherwise
- `reply_id` → Message id to reply to (from get_updates)

The function returns the bot's sent message id, which can be used to edit or delete the message later.

---

## Sending Documents

To send files under 50 MB(Telegram Bot limitation):

```c
void send_document(long long *CID, const char *file_name, const int reply, const int reply_id);
```

Parameters:

- `CID` → Pointer to chat id
- `file_name` → Path to file (can include directories)
- `reply` → 1 to reply, 0 otherwise
- `reply_id` → Message id to reply to

---

## Deleting Messages

```c
void delete_message(long long *CID, const int bot_msgid, long long *group_chat_id);
```

Parameters:

- `CID` → Pointer to chat id
- `bot_msgid` → Bot message id (from send_message)
- `group_chat_id` → Pointer to group chat id (if applicable)

---

## Editing Messages

```c
void edit_message(long long *CID, const int edit_msg_id, const char *message);
```

Parameters:

- `CID` → Pointer to chat id
- `edit_msg_id` → Bot message id to edit (from send_message)
- `message` → New message text

---

## Pull Requests & Contributions

If you'd like to contribute, feel free to fork the repo and submit a pull request.

### Before You Start

- Ensure your code follows a clean and readable C coding style.
- Keep functions modular and well-documented using comments.
- **Do not** include your own bot token or any sensitive data in commits.
- Test your code thoroughly before submitting a pull request.
- Use descriptive function/variable names and meaningful commit messages.

We appreciate your contributions to improve this project!

## PRs That Will Be Closed

Pull requests will be closed without merging if they:

- Add unnecessary or unrelated features
- Contain broken, untested, or buggy code
- Include hardcoded sensitive data (e.g., bot tokens)
- Do not follow basic C coding style or formatting guidelines

Please make sure your contributions adhere to the project standards to avoid delays or closures.

## Creating Issues

If you encounter bugs, have feature requests, or want to suggest improvements, please open an issue on the repository.

### When creating an issue, please include:

- A clear and descriptive title.
- A detailed description of the problem or suggestion.
- Steps to reproduce the bug (if applicable).
- Your environment details (OS, compiler version, etc.).
- Any relevant code snippets or error messages.

This helps us understand and address your issue more efficiently. Thank you for helping improve the project!
---

## 📄 License

This project is licensed under the **MIT License** — see the [LICENSE](LICENSE) file for details.