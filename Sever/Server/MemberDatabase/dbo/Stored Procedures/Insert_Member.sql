-- =============================================
-- Create date: <2025-08-06>
-- Description:	<Checking Correct Member>
-- =============================================
CREATE PROCEDURE Insert_Member
	@ID varchar(30),
	@PW varchar(30)
AS
BEGIN
	SET NOCOUNT ON;

	DECLARE @UserName NVARCHAR(128);
    SET @UserName = CURRENT_USER;

	INSERT INTO MemberDatabase.dbo.Member values(@ID, @PW, GETDATE(), @UserName)
END
