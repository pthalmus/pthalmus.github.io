CREATE TABLE [dbo].[Table_User] (
    [Dbkey]           INT          IDENTITY (1, 1) NOT NULL,
    [USERID]          VARCHAR (30) NOT NULL,
    [RegDate]         DATETIME     NOT NULL,
    [LastAccessDate]  DATETIME     NOT NULL,
    [SanctionsStatus] SMALLINT     DEFAULT ((0)) NOT NULL,
    [SanctionsTime]   DATETIME     NOT NULL,
    PRIMARY KEY CLUSTERED ([Dbkey] ASC)
);

