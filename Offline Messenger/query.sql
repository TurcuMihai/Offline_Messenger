--CREATE TABLE utilizatori ( name TEXT NOT NULL, password TEXT NOT NULL , status TEXT NOT NULL);

--INSERT INTO utilizatori (name,password,status) VALUES ('costica','123','offline');

--CREATE TABLE offline_messages (id INT NOT NULL,sender TEXT NOT NULL, receiver TEXT NOT NULL, text TEXT NOT NULL,answer_for INT);

--INSERT INTO offline_messages(id,sender,receiver,text,answer_for) VALUES (4,'mioara','mimi','Bravo',NULL);

--UPDATE utilizatori set status = 'offline' where name = 'mimi';

--drop table if exists messages;

--CREATE TABLE messages (id INT NOT NULL, sender TEXT NOT NULL, receiver TEXT NOT NULL, text TEXT NOT NULL, answer_for);

--INSERT into messages (id,sender,receiver,text,answer_for) VALUES (3,'mimi','mioara','desenez',1);

--Drop table if exists offline_messages;
