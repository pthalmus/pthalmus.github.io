CREATE TABLE [dbo].[Table_Alert] (
    [Owner]     INT           NOT NULL,
    [Slot]      INT           NOT NULL,
    [Deleted]   INT           NOT NULL,
    [AlertType] SMALLINT      NOT NULL,
    [AlertData] VARCHAR (256) DEFAULT ('') NOT NULL
);

